
#include "tiki/renderer/rendereffect.hpp"

#include "tiki/base/assert.hpp"

namespace tiki
{
	RenderEffect::RenderEffect()
	{
		m_pFrameData		= nullptr;
		m_pRendererContext	= nullptr;
	}

	RenderEffect::~RenderEffect()
	{
		TIKI_ASSERT( m_pFrameData == nullptr );
		TIKI_ASSERT( m_pRendererContext == nullptr );
	}

	bool RenderEffect::create( const RendererContext& rendererContext, GraphicsSystem& graphicsSystem, ResourceRequestPool& resourceRequestPool )
	{
		m_pRendererContext = &rendererContext;

		return createInternal( graphicsSystem, resourceRequestPool );
	}

	bool RenderEffect::createShaderResources( GraphicsSystem& graphicsSystem, ResourceRequestPool& resourceRequestPool )
	{
		return createShaderResourcesInternal( graphicsSystem, resourceRequestPool );
	}

	void RenderEffect::dispose( GraphicsSystem& graphicsSystem, ResourceRequestPool& resourceRequestPool )
	{
		disposeInternal( graphicsSystem, resourceRequestPool );

		m_pFrameData		= nullptr;
		m_pRendererContext	= nullptr;
	}

	void RenderEffect::setFrameData( const FrameData& frameData )
	{
		TIKI_ASSERT( m_pRendererContext != nullptr );
		m_pFrameData = &frameData;
	}

	void RenderEffect::executeRenderSequences( GraphicsContext& graphisContext, RenderPass pass, const RenderSequence* pSequences, uint sequenceCount )
	{
		TIKI_ASSERT( m_pFrameData != nullptr );
		TIKI_ASSERT( m_pRendererContext != nullptr );

		executeRenderSequencesInternal( graphisContext, pass, pSequences, sequenceCount, *m_pFrameData, *m_pRendererContext );
	}
}