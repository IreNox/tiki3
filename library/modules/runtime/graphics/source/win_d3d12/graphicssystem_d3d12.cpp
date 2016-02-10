
#include "tiki/graphics/graphicssystem.hpp"

#include "tiki/base/assert.hpp"
#include "tiki/base/crc32.hpp"
#include "tiki/base/memory.hpp"
#include "tiki/graphics/color.hpp"
#include "tiki/graphics/vertexformat.hpp"

#include "graphicssystem_internal_d3d12.hpp"

namespace tiki
{
	namespace graphics
	{
		static bool initDevice( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params );
		static bool initSwapChain( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params, const uint2& backBufferSize );
		static bool initFrame( GraphicsSystemPlatformData& data, GraphicsSystemFrame& frame, uint frameIndex, DescriptorHandle backBufferColorHandle );
		static bool initObjects( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params );
		static bool initWaitForGpu( GraphicsSystemPlatformData& data );

		static void resetDeviceState( const GraphicsSystemPlatformData& data );
	}

	bool GraphicsSystem::createPlatform( const GraphicsSystemParameters& params )
	{
		uint2 backBufferSize;
		backBufferSize.x = (uint32)params.backBufferWidth;
		backBufferSize.y = (uint32)params.backBufferHeight;

		m_frameNumber = 0;

		if( params.fullScreen )
		{
			RECT rect;
			HWND hDesktop = GetDesktopWindow();
			GetWindowRect( hDesktop, &rect );

			backBufferSize.x = (rect.left - rect.right);
			backBufferSize.y = (rect.top - rect.bottom);
		}

		if( !graphics::initDevice( m_platformData, params ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create Device.\n" );
			disposePlatform();
			return false;
		}

		for( size_t i = 0u; i < TIKI_COUNT( m_platformData.frames ); ++i )
		{
			if( FAILED( m_platformData.pDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_platformData.frames[ i ].pCommandAllocator ) ) ) )
			{
				TIKI_TRACE_ERROR( "[graphics] Could not create CommandAllocator.\n" );
				disposePlatform();
				return false;
			}
		}

		if( !graphics::initObjects( m_platformData, params ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create D3D Objects.\n" );
			disposePlatform();
			return false;
		}

		if( !graphics::initSwapChain( m_platformData, params, backBufferSize ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create SwapChain.\n" );
			disposePlatform();
			return false;
		}

		if( !m_platformData.uploadHeap.create( m_platformData.pDevice, GraphicsSystemLimits_MaxUploadHeapSize ) ||
			!m_platformData.shaderResourcePool.create( m_platformData.pDevice, GraphicsSystemLimits_MaxShaderResourceViews, DescriptorType_ShaderResource ) ||
			!m_platformData.samplerPool.create( m_platformData.pDevice, GraphicsSystemLimits_MaxSamplers, DescriptorType_Sampler ) ||
			!m_platformData.renderTargetPool.create( m_platformData.pDevice, GraphicsSystemLimits_MaxRenderTargetViews, DescriptorType_RenderTarget ) ||
			!m_platformData.depthStencilPool.create( m_platformData.pDevice, GraphicsSystemLimits_MaxDepthStencilViews, DescriptorType_DepthStencil ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create upload heap or descriptor pools.\n" );
			disposePlatform();
			return false;
		}

		// create back buffer target
		{
			TextureDescription depthDescription;
			depthDescription.width		= backBufferSize.x;
			depthDescription.height		= backBufferSize.y;
			depthDescription.format		= PixelFormat_Depth24Stencil8;
			depthDescription.flags		= TextureFlags_DepthStencil;
			depthDescription.type		= TextureType_2d;

			if( !m_backBufferDepthData.create( *this, depthDescription ) )
			{
				TIKI_TRACE_ERROR( "[graphics] Could not create DepthStencilBuffer.\n" );
				disposePlatform();
				return false;
			}

			TextureDescription colorDescription;
			colorDescription.width		= backBufferSize.x;
			colorDescription.height		= backBufferSize.y;
			colorDescription.format		= PixelFormat_R8G8B8A8;
			colorDescription.flags		= TextureFlags_RenderTarget;
			colorDescription.type		= TextureType_2d;

			m_backBufferColorData.m_description = colorDescription;
			HRESULT result = m_platformData.pSwapChain->GetBuffer( 0, __uuidof(ID3D12Resource), (void**)&m_backBufferColorData.m_platformData.pResource );
			if( FAILED( result ) )
			{
				TIKI_TRACE_ERROR( "[graphics] Could not get BackBuffer.\n" );
				disposePlatform();
				return false;
			}

			RenderTargetBuffer colorBuffer( m_backBufferColorData );
			RenderTargetBuffer depthBuffer( m_backBufferDepthData );
			if( !m_backBufferTarget.create( *this, backBufferSize.x, backBufferSize.y, &colorBuffer, 1u, &depthBuffer ) )
			{
				TIKI_TRACE_ERROR( "[graphics] Could not create BackBuffer/DepthStencilBuffer.\n" );
				disposePlatform();
				return false;
			}
		}

		for( size_t i = 0u; i < TIKI_COUNT( m_platformData.frames ); ++i )
		{
			DescriptorHandle backBufferColorHandle = (i == 0u ? m_backBufferTarget.m_platformData.colorHandles[ 0u ] : InvalidDescriptorHandle);
			if( !graphics::initFrame( m_platformData, m_platformData.frames[ i ], i, backBufferColorHandle ) )
			{
				disposePlatform();
				return false;
			}
		}

		if ( !m_commandBuffer.create( *this ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create CommandBuffer.\n" );
			disposePlatform();
			return false;
		}

		if ( !graphics::initWaitForGpu( m_platformData ) )
		{
			disposePlatform();
			return false;
		}

		return true;
	}

	void GraphicsSystem::disposePlatform()
	{
		m_backBufferTarget.dispose( *this );
		m_backBufferDepthData.dispose( *this );
		GraphicsSystemPlatform::safeRelease( &m_backBufferColorData.m_platformData.pResource );

		m_commandBuffer.dispose( *this );
		
		if ( m_platformData.waitEventHandle != INVALID_HANDLE_VALUE )
		{
			// wait for the GPU to be done with all resources
			const UINT64 fenceToWait = m_platformData.currentFench;
			const UINT64 lastCompletedFence = m_platformData.pFence->GetCompletedValue();

			// Signal and increment the current fence
			m_platformData.pCommandQueue->Signal( m_platformData.pFence, m_platformData.currentFench);
			m_platformData.currentFench++;

			if ( lastCompletedFence < fenceToWait )
			{
				TIKI_VERIFY( m_platformData.pFence->SetEventOnCompletion( fenceToWait, m_platformData.waitEventHandle ) );
				WaitForSingleObject( m_platformData.waitEventHandle, INFINITE );
			}

			CloseHandle( m_platformData.waitEventHandle );
		}

		if( m_platformData.pSwapChain != nullptr )
		{
			m_platformData.pSwapChain->SetFullscreenState( false, nullptr );
		}

		m_platformData.uploadHeap.dispose();

		for( size_t i = 0u; i < TIKI_COUNT( m_platformData.frames ); ++i )
		{
			GraphicsSystemFrame& frame = m_platformData.frames[ i ];

			GraphicsSystemPlatform::safeRelease( &frame.pCommandAllocator );
			GraphicsSystemPlatform::safeRelease( &frame.pBackBufferColorResouce );

			if( i != 0u && frame.backBufferColorHandle != InvalidDescriptorHandle )
			{
				m_platformData.renderTargetPool.freeDescriptor( frame.backBufferColorHandle );
				frame.backBufferColorHandle = InvalidDescriptorHandle;
			}
		}

		m_platformData.shaderResourcePool.dispose();
		m_platformData.samplerPool.dispose();
		m_platformData.renderTargetPool.dispose();
		m_platformData.depthStencilPool.dispose();

		GraphicsSystemPlatform::safeRelease( &m_platformData.pCommandList );
		GraphicsSystemPlatform::safeRelease( &m_platformData.pRootSignature );
		GraphicsSystemPlatform::safeRelease( &m_platformData.pCommandQueue );
		GraphicsSystemPlatform::safeRelease( &m_platformData.pDevice );
		GraphicsSystemPlatform::safeRelease( &m_platformData.pSwapChain );
		GraphicsSystemPlatform::safeRelease( &m_platformData.pFactory );
	}

	bool GraphicsSystem::resize( uint width, uint height )
	{
		if( m_backBufferTarget.getWidth() == width && m_backBufferTarget.getHeight() == height )
		{
			return true;
		}

		if ( width == 0u || height == 0u )
		{
			return false;
		}

		m_platformData.pCommandList->OMSetRenderTargets( 0u, nullptr, FALSE, nullptr );

		m_backBufferTarget.dispose( *this );
		m_backBufferDepthData.dispose( *this );
		GraphicsSystemPlatform::safeRelease( &m_backBufferColorData.m_platformData.pResource );

		HRESULT result = m_platformData.pSwapChain->ResizeBuffers( 0, UINT( width ), UINT( height ), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT );
		if ( FAILED( result ) )
		{
			dispose();
			return false;
		}

		TextureDescription depthDescription;
		depthDescription.width		= (uint16)width;
		depthDescription.height		= (uint16)height;
		depthDescription.format		= PixelFormat_Depth24Stencil8;
		depthDescription.flags		= TextureFlags_DepthStencil;
		depthDescription.type		= TextureType_2d;

		if( !m_backBufferDepthData.create( *this, depthDescription ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create DepthStencilBuffer.\n" );
			return false;
		}

		result = m_platformData.pSwapChain->GetBuffer( 0, __uuidof(ID3D12Resource), (void**)&m_backBufferColorData.m_platformData.pResource );
		if( FAILED( result ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not get BackBuffer.\n" );
			return false;
		}

		RenderTargetBuffer colorBuffer( m_backBufferColorData );
		RenderTargetBuffer depthBuffer( m_backBufferDepthData );
		if( !m_backBufferTarget.create( *this, width, height, &colorBuffer, 1u, &depthBuffer ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not create BackBuffer/DepthStencilBuffer.\n" );
			disposePlatform();
			return false;
		}

		return true;
	}

	GraphicsContext& GraphicsSystem::beginFrame()
	{
		const UINT64 lastCompletedFence = m_platformData.pFence->GetCompletedValue();
		if (m_platformData.currentFench > 1u && m_platformData.currentFench > lastCompletedFence)
		{
			TIKI_VERIFY( SUCCEEDED(m_platformData.pFence->SetEventOnCompletion( m_platformData.currentFench, m_platformData.waitEventHandle ) ) );
			WaitForSingleObject( m_platformData.waitEventHandle, INFINITE );
		}
			
		TIKI_VERIFY( SUCCEEDED( m_platformData.pCommandList->Close() ) );

		ID3D12CommandList* pCommandList = m_platformData.pCommandList;
		m_platformData.pCommandQueue->ExecuteCommandLists( 1u, &pCommandList );

		m_frameNumber++;
		m_platformData.currentSwapBufferIndex = m_frameNumber % GraphicsSystemLimits_MaxFrameCount;

		GraphicsSystemFrame& frame = m_platformData.frames[ m_platformData.currentSwapBufferIndex ];

		m_backBufferColorData.m_platformData.pResource = frame.pBackBufferColorResouce;
		m_backBufferTarget.m_platformData.colorHandles[ 0u ] = frame.backBufferColorHandle;

		TIKI_VERIFY( SUCCEEDED( frame.pCommandAllocator->Reset() ) );
		TIKI_VERIFY( SUCCEEDED( m_platformData.pCommandList->Reset( frame.pCommandAllocator, nullptr ) ) );

		GraphicsSystemPlatform::setResourceBarrier(
			m_platformData.pCommandList,
			frame.pBackBufferColorResouce,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		m_platformData.pCommandList->SetGraphicsRootSignature( m_platformData.pRootSignature );

		return m_commandBuffer;
	}

	void GraphicsSystem::endFrame()
	{
		GraphicsSystemFrame& frame = m_platformData.frames[ m_platformData.currentSwapBufferIndex ];

		GraphicsSystemPlatform::setResourceBarrier(
			m_platformData.pCommandList,
			frame.pBackBufferColorResouce,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		//TIKI_VERIFY( SUCCEEDED( m_platformData.pCommandList->Close() ) );
		//ID3D12CommandList* apCommandList[] = { m_platformData.pCommandList };
		//m_platformData.pCommandQueue->ExecuteCommandLists( TIKI_COUNT( apCommandList ), apCommandList );
		
		TIKI_VERIFY( SUCCEEDED( m_platformData.pSwapChain->Present( 1, 0 ) ) );
		
		m_platformData.uploadHeap.finalizeFrame( (uint)m_platformData.currentFench );

		// signal and increment the current fence
		m_platformData.pCommandQueue->Signal( m_platformData.pFence, m_platformData.currentFench );
		m_platformData.currentFench++;

		graphics::resetDeviceState( m_platformData );
	}

	static bool graphics::initDevice( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params )
	{
#if TIKI_ENABLED( TIKI_BUILD_DEBUG )
		ID3D12Debug* pDebug = nullptr;
		if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &pDebug ) ) ) )
		{
			pDebug->EnableDebugLayer();
			GraphicsSystemPlatform::safeRelease( &pDebug );
		}
#endif

		if( FAILED( CreateDXGIFactory1( IID_PPV_ARGS( &data.pFactory ) ) ) )
		{
			return false;
		}

		IDXGIAdapter1* pAdapter = nullptr;
		IDXGIAdapter1* pCurrentAdapter = nullptr;
		for( UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != data.pFactory->EnumAdapters1( adapterIndex, &pCurrentAdapter ); ++adapterIndex )
		{
			DXGI_ADAPTER_DESC1 desc;
			pCurrentAdapter->GetDesc1( &desc );

			if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
			{
				continue;
			}

			pAdapter = pCurrentAdapter;
		}

		HRESULT result = D3D12CreateDevice(
			pAdapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS( &data.pDevice )
		);

		GraphicsSystemPlatform::safeRelease( &pAdapter );

		if( FAILED( result ) )
		{
			return false;
		}
		
		return true;
	}

	static bool graphics::initSwapChain( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params, const uint2& backBufferSize )
	{
		TIKI_DECLARE_STACKANDZERO( DXGI_SWAP_CHAIN_DESC1, swapChainDesc );
		swapChainDesc.BufferCount		= 2u;
		swapChainDesc.Width				= backBufferSize.x;
		swapChainDesc.Height			= backBufferSize.y;
		swapChainDesc.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count	= 1;
		swapChainDesc.Flags				= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		TIKI_DECLARE_STACKANDZERO( DXGI_SWAP_CHAIN_FULLSCREEN_DESC, fullscreenDesc );
		fullscreenDesc.Windowed = !params.fullScreen;
		
		data.swapBufferCount = swapChainDesc.BufferCount;

		HRESULT result = data.pFactory->CreateSwapChainForHwnd(
			data.pCommandQueue,
			(HWND)params.pWindowHandle,
			&swapChainDesc,
			nullptr,
			nullptr,
			&data.pSwapChain
		);

		return SUCCEEDED( result );
	}

	bool graphics::initFrame( GraphicsSystemPlatformData& data, GraphicsSystemFrame& frame, uint frameIndex, DescriptorHandle backBufferColorHandle )
	{
		HRESULT result = data.pSwapChain->GetBuffer( (UINT)frameIndex, IID_PPV_ARGS( &frame.pBackBufferColorResouce ) );
		if( FAILED( result ) )
		{
			TIKI_TRACE_ERROR( "[graphics] Could not get BackBuffer.\n" );
			return false;
		}

		if( backBufferColorHandle != InvalidDescriptorHandle )
		{
			frame.backBufferColorHandle = backBufferColorHandle;
		}
		else
		{
			frame.backBufferColorHandle = data.renderTargetPool.allocateDescriptor();
			if( frame.backBufferColorHandle == InvalidDescriptorHandle )
			{
				return false;
			}

			data.pDevice->CreateRenderTargetView( frame.pBackBufferColorResouce, nullptr, data.renderTargetPool.getCpuHandle( frame.backBufferColorHandle ) );
		}
		
		return true;
	}
	
	static bool graphics::initObjects( GraphicsSystemPlatformData& data, const GraphicsSystemParameters& params )
	{
		GraphicsSystemFrame& frame = data.frames[ data.currentSwapBufferIndex ];

		TIKI_DECLARE_STACKANDZERO( D3D12_COMMAND_QUEUE_DESC, queueDesc );
		queueDesc.Flags	= D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type	= D3D12_COMMAND_LIST_TYPE_DIRECT;
		if ( FAILED( data.pDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &data.pCommandQueue ) ) ) )
		{
			return false;
		}

		if( FAILED( data.pDevice->CreateCommandList( 1u, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.pCommandAllocator, nullptr, IID_PPV_ARGS( &data.pCommandList ) ) ) )
		{
			return false;
		}

		CD3DX12_DESCRIPTOR_RANGE descRanges[ 3u ];
		descRanges[ 0u ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GraphicsSystemLimits_VertexShaderTextureSlots + GraphicsSystemLimits_PixelShaderTextureSlots, 0 );
		descRanges[ 1u ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, GraphicsSystemLimits_VertexShaderTextureSlots + GraphicsSystemLimits_PixelShaderTextureSlots, 0 );
		descRanges[ 2u ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GraphicsSystemLimits_VertexShaderConstantSlots + GraphicsSystemLimits_PixelShaderConstantSlots, 0 );

		CD3DX12_ROOT_PARAMETER rootParameters[ 3u ];
		rootParameters[ 0u ].InitAsDescriptorTable( 1u, &descRanges[ 0u ], D3D12_SHADER_VISIBILITY_ALL );
		rootParameters[ 1u ].InitAsDescriptorTable( 1u, &descRanges[ 1u ], D3D12_SHADER_VISIBILITY_ALL );
		rootParameters[ 2u ].InitAsDescriptorTable( 1u, &descRanges[ 2u ], D3D12_SHADER_VISIBILITY_ALL );

		CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
		descRootSignature.Init( TIKI_COUNT( rootParameters ), rootParameters, 0u, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

		ID3DBlob* pOutputBlob	= nullptr;
		ID3DBlob* pErrorBlob	= nullptr;
		if( FAILED( D3D12SerializeRootSignature( &descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutputBlob, &pErrorBlob ) ) )
		{
			TIKI_ASSERT( pOutputBlob == nullptr );

			if( pErrorBlob != nullptr )
			{
				TIKI_TRACE_ERROR( "Failed to serialize RootSignature. Error: %s", (const char*)pErrorBlob->GetBufferPointer() );
				pErrorBlob->Release();
				pErrorBlob = nullptr;
			}

			return false;
		}
		TIKI_ASSERT( pOutputBlob != nullptr );
		TIKI_ASSERT( pErrorBlob == nullptr );

		HRESULT result = data.pDevice->CreateRootSignature( 1u, pOutputBlob->GetBufferPointer(), pOutputBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&data.pRootSignature );

		pOutputBlob->Release();
		pOutputBlob = nullptr;

		if( FAILED( result ) )
		{
			return false;
		}

		result = data.pDevice->CreateFence( 0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &data.pFence ) );
		if ( FAILED( result ) )
		{
			return false;
		}

		return true;
	}

	bool graphics::initWaitForGpu( GraphicsSystemPlatformData& data )
	{
		data.waitEventHandle = CreateEventEx( nullptr, false, false, EVENT_ALL_ACCESS );

		const uint64 fenchToWait = data.currentFench;
		if ( FAILED( data.pCommandQueue->Signal( data.pFence, fenchToWait ) ) )
		{
			return false;
		}
		data.currentFench++;

		if ( FAILED( data.pFence->SetEventOnCompletion( fenchToWait, data.waitEventHandle ) ) )
		{
			return false;
		}
		WaitForSingleObject( data.waitEventHandle, INFINITE );

		return true;
	}

	void graphics::resetDeviceState( const GraphicsSystemPlatformData& data )
	{
		//static void* s_pNullPointers[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_VertexShaderConstantSlots );
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_VertexShaderTextureSlots );
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_PixelShaderConstantSlots );
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_PixelShaderTextureSlots );
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_MaxInputStreams );
		//TIKI_COMPILETIME_ASSERT( TIKI_COUNT( s_pNullPointers ) >= GraphicsSystemLimits_RenderTargetSlots );
		//TIKI_COMPILETIME_ASSERT( sizeof( void* ) >= sizeof( UINT ) );
		//
		//data.pCommandList->Reset( data.pCommandAllocator, )

		//pContext->VSSetConstantBuffers( 0u, GraphicsSystemLimits_VertexShaderConstantSlots, reinterpret_cast<ID3D11Buffer**>(s_pNullPointers) );
		//pContext->VSSetShaderResources( 0u, GraphicsSystemLimits_VertexShaderTextureSlots, reinterpret_cast<ID3D11ShaderResourceView**>(s_pNullPointers) );
		//pContext->PSSetConstantBuffers( 0u, GraphicsSystemLimits_PixelShaderConstantSlots, reinterpret_cast<ID3D11Buffer**>(s_pNullPointers) );
		//pContext->PSSetShaderResources( 0u, GraphicsSystemLimits_PixelShaderTextureSlots, reinterpret_cast<ID3D11ShaderResourceView**>(s_pNullPointers) );
		//pContext->IASetVertexBuffers( 0u, GraphicsSystemLimits_MaxInputStreams, reinterpret_cast<ID3D11Buffer**>(s_pNullPointers), reinterpret_cast<UINT*>(s_pNullPointers), reinterpret_cast<UINT*>(s_pNullPointers) );
		//pContext->OMSetRenderTargets( 0u, nullptr, nullptr );

		TIKI_NOT_IMPLEMENTED;
	}
}