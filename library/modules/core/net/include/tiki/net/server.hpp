#pragma once
#ifndef TIKI_TIKIXML_HPP
#define TIKI_TIKIXML_HPP

#include "tiki/base/array.hpp"
#include "tiki/base/string.hpp"
#include "tiki/base/types.hpp"

#include "xml.h"

namespace tiki
{
	typedef Array< const XmlElement* > XmlElementList;

	class XmlReader
	{
		TIKI_NONCOPYABLE_CLASS( XmlReader );
		friend void* xmlAlloc(size_t _bytes, void* pUserData);

	public:

							XmlReader();
							~XmlReader();

		bool				create( cstring pFileName );
		void				dispose();

		const XmlElement*	getRoot() const { return m_pNode; }

		const XmlElement*	findNodeByName( cstring pName ) const;
		const XmlElement*	findFirstChild( cstring pName,  const XmlElement* pElement ) const;
		const XmlElement*	findNext( const XmlElement* pElement ) const;
		const XmlElement*	findNext( cstring pName, const XmlElement* pElement ) const;

		const XmlAttribute*	findAttributeByName( cstring pName, const XmlElement* pElement ) const;

		size_t				getChilds( XmlElementList& targetList, const XmlElement* pElement, cstring pName ) const;

	private:

		XmlElement*			m_pNode;
		void*				m_pData;

	};
}


#endif // TIKI_TIKIXML_HPP