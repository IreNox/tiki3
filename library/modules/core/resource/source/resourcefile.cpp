
#include "tiki/resource/resourcefile.hpp"

#include "tiki/base/functions.hpp"

namespace tiki
{
	Endianness resource::getEndianness( const ResourceFileHeader& header )
	{
		return Endianness_Little;
	}

	const AllocatorType resource::getSectionAllocatorType( uint8 data )
	{
		return (AllocatorType)( data >> 6u );
	}

	const uint8 resource::getSectionAllocatorId( uint8 data )
	{
		return ( data & 0x3fu );
	}

	StringType resource::getStringType( const StringItem& item )
	{
		return (StringType)( ( item.type_lengthModifier_textLength & 0xc0000000u ) >> 30u );
	}

	uint resource::getStringLengthModifier( const StringItem& item )
	{
		return ( ( item.type_lengthModifier_textLength & 0x40000000u ) >> 28u );
	}

	uint resource::getStringTextLength( const StringItem& item )
	{
		return ( item.type_lengthModifier_textLength & 0x0fffffffu );
	}

	uint resource::getStringLengthInBytes( const StringItem& item )
	{
		return getStringTextLength( item ) * getStringLengthModifier( item );
	}
}