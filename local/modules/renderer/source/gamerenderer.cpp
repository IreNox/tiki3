
#include "tiki/renderer/gamerenderer.hpp"

#include "tiki/graphics/graphicscontext.hpp"

namespace tiki
{
	GameRenderer::GameRenderer()
	{
	}

	GameRenderer::~GameRenderer()
	{
	}

	bool GameRenderer::create( GraphicsSystem& graphicsSystem, const GameRendererParamaters& parameters )
	{
		m_context.pGraphicsSystem	= &graphicsSystem;
		m_context.rendererWidth		= parameters.rendererWidth;
		m_context.rendererHeight	= parameters.rendererHeight;

		if ( m_renderBatch.create( parameters.maxSeqeuenceCount, parameters.maxRenderCommandCount ) == false )
		{
			return false;
		}

		if ( m_renderEffectSystem.create( m_context ) == false )
		{
			dispose();
			return false;
		}

		if ( createTextureData() == false )
		{
			dispose();
			return false;
		}

		if ( createRenderTargets() == false )
		{
			dispose();
			return false;
		}

		return true;
	}

	void GameRenderer::dispose()
	{
		disposeRenderTargets();
		disposeTextureData();

		m_renderEffectSystem.dispose();
		m_renderBatch.dispose();
	}

	void GameRenderer::update()
	{
		m_renderEffectSystem.setFrameData( m_frameData );
	}

	void GameRenderer::render( GraphicsContext& graphicsContext ) const
	{
		graphicsContext.beginRenderPass( m_geometryTarget );

		m_renderEffectSystem.render( graphicsContext, RenderPass_Geometry, m_renderBatch );

		graphicsContext.endRenderPass();
	}

	bool GameRenderer::createTextureData()
	{
		GraphicsSystem& graphicsSystem = *m_context.pGraphicsSystem;

		TextureDescription desc;
		desc.width		= m_context.rendererWidth;
		desc.height		= m_context.rendererHeight;
		desc.depth		= 1u;
		desc.type		= TextureType_2d;
		desc.mipCount	= 0u;
		desc.arrayCount	= 1u;

		desc.format		= PixelFormat_Depth24Stencil8;
		desc.flags		= TextureFlags_DepthStencil;
		if ( m_depthBuffer.create( graphicsSystem, desc ) == false ) return false;

		desc.flags		= TextureFlags_ShaderInput;
		if ( m_readyOnlyDepthBuffer.create( graphicsSystem, desc ) == false ) return false;

		desc.format		= PixelFormat_R8G8B8A8_Gamma;
		desc.flags		= TextureFlags_ShaderInput | TextureFlags_RenderTarget;
		if ( m_geometryBufferData[ 0u ].create( graphicsSystem, desc ) == false ) return false;
		if ( m_geometryBufferData[ 1u ].create( graphicsSystem, desc ) == false ) return false;

		desc.format		= PixelFormat_R16G16B16A16_Float;
		if ( m_geometryBufferData[ 2u ].create( graphicsSystem, desc ) == false ) return false;

		desc.format		= PixelFormat_R8G8B8A8_Gamma;
		if ( m_accumulationData.create( graphicsSystem, desc ) == false ) return false;

		return true;
	}

	bool GameRenderer::createRenderTargets()
	{
		GraphicsSystem& graphicsSystem = *m_context.pGraphicsSystem;
		const uint width	= m_context.rendererWidth;
		const uint height	= m_context.rendererHeight;

		// geometry buffer
		{
			RenderTargetBuffer depthBuffer( m_depthBuffer );
			RenderTargetBuffer colorBuffers[] =
			{
				RenderTargetBuffer( m_geometryBufferData[ 0u ] ),
				RenderTargetBuffer( m_geometryBufferData[ 1u ] ),
				RenderTargetBuffer( m_geometryBufferData[ 2u ] )
			};

			if ( m_geometryTarget.create( graphicsSystem, width, height, colorBuffers, TIKI_COUNT( colorBuffers ), &depthBuffer ) == false )
			{
				return false;
			}
		}

		// accumulation buffer
		{
			RenderTargetBuffer depthBuffer( m_depthBuffer );
			RenderTargetBuffer colorBuffers[] =
			{
				RenderTargetBuffer( m_accumulationData )
			};

			if ( m_accumulationTarget.create( graphicsSystem, width, height, colorBuffers, TIKI_COUNT( colorBuffers ), &depthBuffer ) == false )
			{
				return false;
			}
		}

		return true;
	}

	void GameRenderer::disposeTextureData()
	{
		GraphicsSystem& graphicsSystem = *m_context.pGraphicsSystem;
		
		m_depthBuffer.dispose( graphicsSystem );
		m_readyOnlyDepthBuffer.dispose( graphicsSystem );

		for (uint i = 0u; i < TIKI_COUNT( m_geometryBufferData ); ++i)
		{
			m_geometryBufferData[ i ].dispose( graphicsSystem );
		}		

		m_accumulationData.dispose( graphicsSystem );
	}

	void GameRenderer::disposeRenderTargets()
	{
		GraphicsSystem& graphicsSystem = *m_context.pGraphicsSystem;

	}
}