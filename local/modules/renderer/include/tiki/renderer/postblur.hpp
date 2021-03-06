#pragma once
#ifndef __TIKI_POSTBLUR_HPP_INCLUDED__
#define __TIKI_POSTBLUR_HPP_INCLUDED__

#include "tiki/base/types.hpp"
#include "tiki/graphics/constantbuffer.hpp"
#include "tiki/graphics/pixelformat.hpp"
#include "tiki/graphics/rendertarget.hpp"
#include "tiki/graphics/texturedata.hpp"

namespace tiki
{
	class BlendState;
	class DepthStencilState;
	class GraphicsContext;
	class GraphicsSystem;
	class RasterizerState;
	class ResourceManager;
	class SamplerState;
	class ShaderSet;
	class VertexInputBinding;

	class PostProcessBlur
	{
		TIKI_NONCOPYABLE_CLASS( PostProcessBlur );

	public:

		PostProcessBlur();
		~PostProcessBlur();

		bool	create( GraphicsSystem& graphicsSystem, ResourceManager& resourceManager, uint width, uint height, PixelFormat format = PixelFormat_Color );
		void	dispose( GraphicsSystem& graphicsSystem, ResourceManager& resourceManager );
		
		bool	resize( GraphicsSystem& graphicsSystem, uint width, uint height );

		void	render( GraphicsContext& graphicsContext, const TextureData& sourceData, const RenderTarget& target ) const;

	private:

		const ShaderSet*			m_pShader;

		PixelFormat					m_format;

		const BlendState*			m_pBlendState;
		const DepthStencilState*	m_pDepthState;
		const RasterizerState*		m_pRasterizerState;
		const SamplerState*			m_pSamplerState;

		const VertexInputBinding*	m_pInputBinding;
		
		TextureData					m_textureData;
		RenderTarget				m_renderTarget;

		ConstantBuffer				m_pixelConstants;
		
		bool						createRenderTargets( GraphicsSystem& graphicsSystem, uint width, uint height );
		void						disposeRenderTargets( GraphicsSystem& graphicsSystem );

	};
}

#endif // __TIKI_POSTBLUR_HPP_INCLUDED__
