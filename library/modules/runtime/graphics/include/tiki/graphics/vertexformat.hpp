#ifndef TIKI_VERTEXFORMAT_HPP
#define TIKI_VERTEXFORMAT_HPP

#include "tiki/container/fixedarray.hpp"
#include "tiki/container/fixedsizedarray.hpp"
#include "tiki/base/types.hpp"
#include "tiki/graphics/graphicsstateobject.hpp"
#include "tiki/graphics/graphicssystemlimits.hpp"
#include "tiki/graphics/vertexattribute.hpp"

namespace tiki
{
	class Shader;

	struct VertexFormatParameters
	{
		const VertexAttribute*	pAttributes;
		size_t					attributeCount;
	};

	class VertexFormat : public GraphicsStateObject
	{
		TIKI_NONCOPYABLE_WITHCTOR_CLASS( VertexFormat );
		friend class GraphicsContext;
		friend class GraphicsSystem;

	public:

		bool					isCreated() const;

		const VertexAttribute&	getAttributeByIndex( uint index ) const { return m_attributes[ index ]; }
		uint					getAttributeCount() const { return m_attributes.getCount(); }

		uint					getVertexStride( uint streamIndex ) const { return m_vertexStride[ streamIndex ]; }

	private: // friend

		void					create( const VertexFormatParameters& creationParameters );
		void					dispose();

	private:

		FixedSizedArray< VertexAttribute, GraphicsSystemLimits_MaxVertexAttributes >	m_attributes;
		FixedSizedArray< uint, GraphicsSystemLimits_MaxInputStreams >					m_vertexStride;

	};
}

#endif // TIKI_VERTEXFORMAT_HPP