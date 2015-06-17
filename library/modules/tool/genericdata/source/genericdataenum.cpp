
#include "tiki/genericdata/genericdataenum.hpp"

#include "tiki/base/stringparse.hpp"
#include "tiki/genericdata/genericdatatypecollection.hpp"
#include "tiki/io/xmlreader.hpp"

namespace tiki
{
	GenericDataEnum::GenericDataEnum( const GenericDataTypeCollection& collection, const string& name, GenericDataTypeMode mode, const GenericDataType& baseType )
		: GenericDataType( collection, name, mode )
		, m_pBaseType( &baseType )
	{
	}

	GenericDataEnum::~GenericDataEnum()
	{
	}

	bool GenericDataEnum::loadFromXml( const XmlReader& reader, const XmlElement* pTypeRoot )
	{
		if ( pTypeRoot == nullptr )
		{
			return false;
		}

		if ( !isStringEquals( pTypeRoot->name, "enum" ) )
		{
			TIKI_TRACE_ERROR( "[GenericDataStruct(%s)::readFromXml] node has a wrong tag('%s' != 'enum') \n", getName().cStr(), pTypeRoot->name );
			return false;
		}

		const XmlElement* pChildElement = pTypeRoot->elements;
		while ( pChildElement != nullptr )
		{
			const bool isValue = isStringEquals( pChildElement->name, "value" );
			if ( isValue )
			{
				const XmlAttribute* pNameAtt = reader.findAttributeByName( "name", pChildElement );
				const XmlAttribute* pModeAtt = reader.findAttributeByName( "mode", pChildElement );
				const XmlAttribute* pValueAtt = reader.findAttributeByName( "value", pChildElement );

				if ( pNameAtt )
				{
					GenericDataEnumValue& field = m_values.add();
					field.name		= pNameAtt->content;
					field.mode		= GenericDataTypeMode_ToolAndRuntime;
					field.hasValue	= false;

					if ( pModeAtt != nullptr )
					{
						GenericDataTypeMode mode = m_collection.findModeByName( pModeAtt->content );
						if ( mode == GenericDataTypeMode_Invalid )
						{
							TIKI_TRACE_WARNING( "[GenericDataStruct(%s)::readFromXml] field or array with name '%s' has a invalid mode attribute. '%s' is not a valid mode.\n", getName().cStr(), pNameAtt->content, pModeAtt->content );
						}
						else
						{
							field.mode = mode;
						}
					}

					if ( pValueAtt != nullptr )
					{
						field.hasValue	= true;
						field.value		= ParseString::parseInt64( pValueAtt->content );
					}
				}
				else
				{
					TIKI_TRACE_ERROR( "[GenericDataStruct(%s)::readFromXml] field or array has not all required attributes. name is required.\n", getName().cStr() );
					return false;
				}				
			}

			pChildElement = pChildElement->next;
		}

		return true;
	}

	bool GenericDataEnum::exportCode( string& targetData, GenericDataTypeMode mode ) const
	{
		static const char* s_pBaseFormat			= "\n"
													  "\tenum %s : %s\n"
													  "\t{\n"
													  "\t\t%s_Invalid = -1,\n"
													  "\n"
													  "%s"
													  "\n"
													  "\t\t%s_Count\n"
													  "\t};\n";

		static const char* s_pValueFormat			= "\t\t%s_%s,\n";
		static const char* s_pValueFormatWithValue	= "\t\t%s_%s = %i,\n";

		string valuesCode;
		for (uint i = 0u; i < m_values.getCount(); ++i)
		{
			const GenericDataEnumValue& value = m_values[ i ];

			if ( value.hasValue )
			{
				valuesCode += formatString( s_pValueFormatWithValue, getName().cStr(), value.name.cStr(), value.value );
			}
			else
			{
				valuesCode += formatString( s_pValueFormat, getName().cStr(), value.name.cStr() );
			}
		}

		targetData += formatString(
			s_pBaseFormat,
			getName().cStr(),
			m_pBaseType->getName().cStr(),
			getName().cStr(),
			valuesCode.cStr(),
			getName().cStr()
		);

		return true;
	}

	GenericDataTypeType GenericDataEnum::getType() const
	{
		return GenericDataTypeType_Enum;
	}

	uint GenericDataEnum::getAlignment() const
	{
		return m_pBaseType->getAlignment();
	}

	uint GenericDataEnum::getSize() const
	{
		return m_pBaseType->getSize();
	}

	const GenericDataType* GenericDataEnum::getBaseType() const
	{
		return m_pBaseType;
	}

	void GenericDataEnum::addValue( const string& name, sint64 value, GenericDataTypeMode mode /*= GenericDataTypeMode_ToolAndRuntime */ )
	{
		GenericDataEnumValue& field = m_values.add();
		field.name	= name;
		field.value	= value;
		field.mode	= mode;
	}

	void GenericDataEnum::removeValue( const string& name )
	{
		for (uint i = 0u; i < m_values.getCount(); ++i)
		{
			if ( m_values[ i ].name == name )
			{
				m_values.removeSortedAtIndex( i );
				return;
			}
		}
	}

	const sint64* GenericDataEnum::getValueByName( const string& name ) const
	{
		for (uint i = 0u; i < m_values.getCount(); ++i)
		{
			if ( m_values[ i ].name == name )
			{
				return &m_values[ i ].value;
			}
		}

		return nullptr;
	}

	const List< GenericDataEnumValue >& GenericDataEnum::getValues() const
	{
		return m_values;
	}
}