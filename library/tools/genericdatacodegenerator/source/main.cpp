
#include "tiki/base/debug.hpp"
#include "tiki/base/platform.hpp"
#include "tiki/base/string.hpp"
#include "tiki/base/types.hpp"
#include "tiki/toolgenericdata/genericdatatypecollection.hpp"

int tiki::mainEntryPoint()
{
	int retValue = 0;

	//debug::breakOnAlloc( 1449 );
	{
		string sourceDir = "../../../../../../content";
		string targetDir = "../../genericdatatypes";

		for (uint i = 0u; i < platform::getArguments().getCount(); ++i)
		{
			const string arg = platform::getArguments()[ i ];

			if ( arg.startsWith( "--content-dir=" ) )
			{
				sourceDir = arg.subString( getStringSize( "--content-dir=" ) );
			}
			else if ( arg.startsWith( "--target-dir=" ) )
			{
				targetDir = arg.subString( getStringSize( "--target-dir=" ) );
			}
		}

		GenericDataTypeCollection collection;
		collection.create( sourceDir, true );
		if ( !collection.exportCode( GenericDataTypeMode_RuntimeOnly, targetDir ) )
		{
			TIKI_TRACE_ERROR( "[genericdatacodegenerator] code generation finish with some errors.\n" );

			retValue = -1;
		}
		else
		{
			TIKI_TRACE_INFO( "[genericdatacodegenerator] code generation successfull.\n" );
		}

		collection.dispose();
	}		
	
	return retValue;
}