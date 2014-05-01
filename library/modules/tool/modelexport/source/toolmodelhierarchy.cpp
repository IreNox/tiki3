
#include "tiki/modelexport/toolmodelhierarchy.hpp"

#include "tiki/toolbase/list.hpp"
#include "tiki/toolbase/tikixml.hpp"

#include "toolmodelsource.hpp"

namespace tiki
{
	ToolModelHierarchy::ToolModelHierarchy()
	{
		m_pXml				= nullptr;
		m_finalJointCount	= TIKI_SIZE_T_MAX;
		m_hasFinalIndices	= false;
	}

	void ToolModelHierarchy::create( const TikiXml* pXml, const _XmlElement* pHierarchyNode, const _XmlElement* pGeometriesNode, float scale )
	{
		TIKI_ASSERT( pXml );
		m_pXml	= pXml;

		List< ToolModelGeometryInstance > instances;
		if ( pHierarchyNode != nullptr )
		{
			List< ToolModelJoint > nodes;
			searchNodes( nodes, pHierarchyNode );

			m_joints.create( nodes.getData(), nodes.getCount() );
			for (size_t i = 0u; i < m_joints.getCount(); ++i)
			{
				ToolModelJoint& joint = m_joints[ i ];

				const XmlAttribute* pIdAtt = pXml->findAttributeByName( "id", joint.pNode );
				TIKI_ASSERT( pIdAtt );

				joint.name	= pIdAtt->content;
				joint.crc	= crcString( joint.name );
				matrix::createIdentity( joint.skinToBone );

				const XmlElement* pMatrix = pXml->findFirstChild( "matrix", joint.pNode );			
				if ( pMatrix == nullptr )
				{
					continue;
				}

				parseMatrix( joint.defaultPose, pMatrix->content, scale );

				const XmlElement* pInstanceNode = pXml->findFirstChild( "instance_geometry", joint.pNode );
				if ( pInstanceNode != nullptr )
				{
					const XmlAttribute* pUrlAtt = pXml->findAttributeByName( "url", pInstanceNode );
					if ( pUrlAtt == nullptr )
					{
						continue;
					}
					const string geometryId = string( pUrlAtt->content ).substring( 1u );

					const XmlElement* pGeometrieNode = pXml->findFirstChild( "geometry", pGeometriesNode );
					while ( pGeometrieNode != nullptr )
					{
						const XmlAttribute* pIdAtt = pXml->findAttributeByName( "id", pGeometrieNode );
						if ( pIdAtt != nullptr && geometryId == pIdAtt->content )
						{
							ToolModelGeometryInstance& instance = instances.add();
							instance.pNode			= pGeometrieNode;
							instance.geometryId		= geometryId;
							instance.worldTransform	= joint.defaultPose;
						}

						pGeometrieNode = pXml->findNext( "geometry", pGeometrieNode );
					}
				}
			}
		}

		if ( instances.getCount() == 0u )
		{
			const XmlElement* pGeometrieNode = pXml->findFirstChild( "geometry", pGeometriesNode );
			while ( pGeometrieNode != nullptr )
			{
				const XmlAttribute* pIdAtt = pXml->findAttributeByName( "id", pGeometrieNode );
				if ( pIdAtt == nullptr )
				{
					continue;
				}

				ToolModelGeometryInstance& instance = instances.add();
				instance.pNode			= pGeometrieNode;
				instance.geometryId		= pIdAtt->content;
				instance.worldTransform	= Matrix44::identity;

				pGeometrieNode = pXml->findNext( "geometry", pGeometrieNode );
			}
		}

		m_instances.create( instances.getData(), instances.getCount() );
	}

	void ToolModelHierarchy::dispose()
	{
		m_pXml = nullptr;
		m_joints.dispose();
		m_instances.dispose();
	}

	const ToolModelJoint* ToolModelHierarchy::getJointByName( const string& name ) const
	{
		const crc32 crc = crcString( name );

		for (size_t i = 0u; i < m_joints.getCount(); ++i)
		{
			const ToolModelJoint& joint = m_joints[ i ];

			if ( joint.crc == crc )
			{
				return &joint;
			}
		}

		return nullptr;
	}

	void ToolModelHierarchy::markJointAsUsed( const ToolModelJoint& constJoint )
	{
		const size_t index		= m_joints.getIndex( constJoint );
		ToolModelJoint& joint	= m_joints[ index ];
		joint.used = true;
	}

	void ToolModelHierarchy::setBindMatrix( const ToolModelJoint& constJoint, const Matrix44& matrix )
	{
		const size_t index		= m_joints.getIndex( constJoint );
		ToolModelJoint& joint	= m_joints[ index ];
		joint.skinToBone = matrix;
	}

	void ToolModelHierarchy::searchNodes( List< ToolModelJoint >& targetList, const XmlElement* pNode )
	{
		const string nodeTag		= "node";
		const XmlElement* pChild	= m_pXml->findFirstChild( nodeTag, pNode );

		size_t parentIndex = targetList.getCount() - 1u;

		while ( pChild )
		{
			ToolModelJoint joint;
			joint.index				= targetList.getCount();
			joint.pNode				= pChild;
			joint.pParentNode		= pNode;
			joint.parentIndex		= parentIndex;
			joint.used				= false;
			joint.finalIndex		= TIKI_SIZE_T_MAX;
			joint.finalParentIndex	= TIKI_SIZE_T_MAX;

			targetList.add( joint );

			searchNodes( targetList, pChild );

			pChild = m_pXml->findNext( nodeTag, pChild );
		}
	}
	
	void ToolModelHierarchy::calculateFinalIndices()
	{
		// mark parents as used
		for (size_t i = 0u; i < m_joints.getCount(); ++i)
		{
			if ( m_joints[ i ].parentIndex != TIKI_SIZE_T_MAX )
			{
				size_t parentIndex = m_joints[ i ].parentIndex;

				while ( parentIndex != TIKI_SIZE_T_MAX )
				{
					m_joints[ parentIndex ].used = true;

					parentIndex = m_joints[ parentIndex ].parentIndex;
				}
			}
		}

		size_t index = 0u;
		Array< size_t > mappingTable;
		mappingTable.create( m_joints.getCount() );

		for (size_t i = 0u; i < m_joints.getCount(); ++i)
		{
			if ( m_joints[ i ].used )
			{
				mappingTable[ i ]				= index;
				m_joints[ i ].finalIndex		= index;
				m_joints[ i ].finalParentIndex	= mappingTable[ m_joints[ i ].parentIndex ];

				index++;
			}
		}

		m_finalJointCount	= index;
		m_hasFinalIndices	= true;

		mappingTable.dispose();
	}
}