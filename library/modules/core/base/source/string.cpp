
#include "tiki/base/string.hpp"

#include <cstdarg>
#include <cstdio>

#if TIKI_ENABLED( TIKI_BUILD_MSVC )
#   include <windows.h>
#endif

namespace tiki
{
#if TIKI_ENABLED( TIKI_BUILD_MSVC )

	bool convertUtf8ToWidecharString( wchar_t* pTargetBuffer, uint targetLengthInCharacters, const char* pSourceBuffer )
	{
		const int result = MultiByteToWideChar(
			CP_UTF8,
			0,
			pSourceBuffer,
			-1,
			pTargetBuffer,
			(int)targetLengthInCharacters
		);

		return result != 0;
	}

	bool convertWidecharToUtf8String( char* pTargetBuffer, uint targetLengthInCharacters, const wchar_t* pSourceBuffer )
	{
		const int result = WideCharToMultiByte(
			CP_UTF8, 0,
			pSourceBuffer,
			-1,
			pTargetBuffer,
			(int)targetLengthInCharacters,
			nullptr,
			nullptr
		);

		return result != 0;
	}

	void formatStringBuffer( char* pTargetBuffer, uint targetLength, const char* format, ... )
	{
		va_list argptr;
		va_start( argptr, format );

		_vsprintf_s_l(
			pTargetBuffer,
			targetLength,
			format,
			nullptr,
			argptr
		);

		va_end( argptr );
	}

#elif TIKI_ENABLED( TIKI_BUILD_GCC ) || TIKI_ENABLED( TIKI_BUILD_CLANG )

	void formatStringBuffer( char* pTargetBuffer, uint bufferSize, const char* format, ... )
	{
		va_list argptr;
		va_start( argptr, format );

		vsnprintf( pTargetBuffer, bufferSize, format, argptr );

		va_end( argptr );
	}

#endif
}