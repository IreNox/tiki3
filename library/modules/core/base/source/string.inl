#pragma once
#ifndef TIKI_STRING_INL
#define TIKI_STRING_INL

#include "tiki/base/memory.hpp"
#include "tiki/base/sizedarray.hpp"

namespace tiki
{
	template<typename TChar>
	struct StringRefData
	{
		TChar* pData;
		uint dataLength;
		uint stringLength;

		sint32 refCount;

		StringRefData()
			: pData( nullptr ), dataLength( 0u ), stringLength( 0u ), refCount( 1 )
		{
		}

		StringRefData( uint strLen, uint dataLen )
			: refCount( 1 )
		{
			pData			= TIKI_NEW TChar[ dataLen ];
			dataLength		= dataLen;
			stringLength	= strLen;

			pData[ strLen ] = 0;
		}

		StringRefData( uint strLen, uint dataLen, const TChar* pBaseData, sint32 baseDataLen = -1 )
			: refCount( 1 )
		{
			pData			= TIKI_NEW TChar[dataLen];
			dataLength		= dataLen;
			stringLength	= strLen;

			memory::copy( pData, pBaseData, sizeof(TChar) * (baseDataLen == -1 ? strLen : baseDataLen) );
			pData[ strLen ] = 0;
		}

		~StringRefData()
		{
			if (pData)
			{
				delete[] pData;
				pData = nullptr;
			}
		}

		TIKI_FORCE_INLINE sint32 addRef()
		{
			return ++refCount;
		}

		TIKI_FORCE_INLINE sint32 release()
		{
			if ( this == &BasicString<TChar>::emptyData ) return 0;

			if (--refCount < 1)
			{
				delete this;
				return 0;
			}

			return refCount;
		}
	};

	template<typename TChar>
	BasicString<TChar>::BasicString()
	{
		data = &emptyData;
	}

	template<typename TChar>
	BasicString<TChar>::BasicString(TChar c)
	{
		data = TIKI_NEW StringRefData<TChar>(1, 2, &c, 1);
	}

	template<typename TChar>
	BasicString<TChar>::BasicString(const TChar* string)
	{
		allocData( string, -1 );
	}

	template<typename TChar>
	BasicString<TChar>::BasicString(const TChar* string, sint32 length)
	{
		allocData( string, length );
	}

	template<typename TChar>
	BasicString<TChar>::BasicString(const BasicString<TChar>& copy)
		: data(copy.data)
	{
		data->addRef();
	}

	template<typename TChar>
	BasicString<TChar>::BasicString( uint  len )
	{
		data = TIKI_NEW StringRefData<TChar>(
			len,
			calcLength(len + 1)
		);
	}

	template<typename TChar>
	BasicString<TChar>::~BasicString()
	{
		if ( data )
		{
			data->release();
			data = nullptr;
		}
	}

	template<typename TChar>
	TIKI_FORCE_INLINE void tiki::BasicString<TChar>::allocData( const TChar* string, sint length = -1 )
	{
		if (string == 0)
		{
			data = &emptyData;
		}
		else
		{
			uint len = (length == -1 ? stringLength(string) : length);

			if ( len == 0)
			{
				data = &emptyData;
				return;
			}

			data = TIKI_NEW StringRefData<TChar>(
				len,
				calcLength(len + 1),
				string
			);
		}
	}

	template<typename TChar>
	TIKI_FORCE_INLINE uint BasicString<TChar>::getLength() const
	{
		return data->stringLength;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::isEmpty() const
	{
		return data->stringLength == 0;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE const TChar* BasicString<TChar>::cStr() const
	{
		return data->pData;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE void BasicString<TChar>::split( Array< BasicString< TChar > >& output, const BasicString<TChar>& seperator ) const
	{
		const uint count = countSubstring( seperator );

		SizedArray< BasicString< TChar > > list;
		list.create( count + 1u );

		uint i = 0;
		uint lastIndex = 0;
		while (i < count)
		{
			const uint index = indexOf(seperator, lastIndex + seperator.data->stringLength);

			list.push(
				substring( lastIndex, index - lastIndex )
			);

			lastIndex = index + seperator.data->stringLength;
			i++;
		}

		if (data->stringLength - lastIndex > 0)
		{
			list.push(
				substring( lastIndex, data->stringLength - lastIndex )
			);
		}

		output.create( list.getData(), list.getCount() );
		list.dispose();
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::replace(TChar oldValue, TChar newValue) const
	{
		BasicString str = *this;

		uint i = 0;
		while (i < data->stringLength)
		{
			if (str[i] == oldValue) str[i] = newValue;
			i++;
		}

		return str;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::replace(const BasicString<TChar>& oldValue, const BasicString<TChar>& newValue) const
	{
		const uint count = countSubstring(oldValue);
		const uint length = data->stringLength - (count * oldValue.data->stringLength) + (count * newValue.data->stringLength);

		if ( count == 0 )
		{
			return *this;
		}

		BasicString<TChar> str = BasicString<TChar>(length);

		uint i = 0;
		uint offsetOld = 0;
		uint offsetNew = 0;
		while (i < count)
		{
			const uint index		= indexOf(oldValue, offsetOld);
			const uint oldDifferent	= index - offsetOld;

			memory::copy(str.data->pData + offsetNew, data->pData + offsetOld, sizeof(TChar) * (index - offsetOld));
			offsetOld += oldDifferent;
			offsetNew += oldDifferent;

			memory::copy(str.data->pData + offsetNew, newValue.data->pData, sizeof(TChar) * newValue.data->stringLength);
			offsetOld += oldValue.data->stringLength;
			offsetNew += newValue.data->stringLength;

			i++;
		}

		memory::copy( str.data->pData + offsetNew, data->pData + offsetOld, sizeof(TChar) * (data->stringLength - offsetOld));

		return str;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::substring(uint startIndex, sint32 length /*= -1*/ ) const
	{
		if (length == -1 || startIndex + length > data->stringLength)
		{
			length = data->stringLength - startIndex;
		}
		TIKI_ASSERT ( startIndex < data->stringLength || length == 0 );

		if ( length == 0 )
		{
			return BasicString< TChar >();
		}

		if ( length == data->stringLength && startIndex == 0u )
		{
			return BasicString<TChar>( *this );
		}

		return BasicString<TChar>(
			data->pData + startIndex,
			length
		);
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::trim() const
	{
		uint start = 0u;
		uint length = data->stringLength;

		if ( length == 0u )
		{
			return *this;
		}

		bool isWhiteSpace = false;
		do 
		{
			isWhiteSpace = false;

			for (uint i = 0u; i < TIKI_COUNT( whiteSpaces ); ++i)
			{
				if ( data->pData[ start ] == whiteSpaces[ i ] )
				{
					isWhiteSpace = true;
					start++;
					length--;
					break;
				}
			}
		}
		while ( isWhiteSpace );

		do 
		{
			isWhiteSpace = false;

			for (uint i = 0u; i < TIKI_COUNT( whiteSpaces ); ++i)
			{
				if ( data->pData[ start + length - 1u ] == whiteSpaces[ i ] )
				{
					isWhiteSpace = true;
					length--;
					break;
				}
			}
		}
		while ( isWhiteSpace );

		return substring( start, length );
	}

	template<typename TChar>
	TIKI_FORCE_INLINE uint BasicString<TChar>::countSubstring(const BasicString<TChar>& str) const
	{
		if (str.data->stringLength > data->stringLength)
			return 0u;

		uint i = 0;
		uint c = 0;
		while (i < data->stringLength)
		{
			uint b = 0;
			bool found = true;
			while (b < str.data->stringLength)
			{
				if (data->pData[i + b] != str.data->pData[b])
				{
					found = false;
					break;
				}
				b++;
			}

			if (found)
			{
				c++;
				i += str.data->stringLength;
			}
			else
			{
				i++;
			}
		}

		return c;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::insert( const BasicString<TChar>& str, uint index ) const
	{
		BasicString<TChar> oStr = BasicString<TChar>(data->stringLength + str.data->stringLength);

		memory::copy(oStr.data->pData, data->pData, sizeof(TChar) * index);
		memory::copy(oStr.data->pData + index, str.data->pData, sizeof(TChar) * str.data->stringLength);
		memory::copy(oStr.data->pData + index + str.data->stringLength, data->pData + index, sizeof(TChar) * (data->stringLength - index));

		return oStr;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::remove( uint startIndex, uint len ) const
	{
		BasicString<TChar> oStr = BasicString<TChar>(data->stringLength - len);

		memory::copy(oStr.data->pData, data->pData, sizeof(TChar) * startIndex);
		memory::copy(oStr.data->pData + startIndex, data->pData + startIndex + len, sizeof(TChar) * (data->stringLength - startIndex - len));

		return oStr;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::toLower() const
	{
		BasicString<TChar> str = *this;

		uint i = 0;
		while (i < data->stringLength)
		{
			if (str[i] >= letterBigA && str[i] <= letterBigZ)
				str[i] -= letterBigZ - letterLittleZ;

			i++;
		}

		return str;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::toUpper() const
	{
		BasicString<TChar> str = *this;

		uint i = 0;
		while (i < data->stringLength)
		{
			if (str[i] >= letterLittleA && str[i] <= letterLittleZ)
				str[i] -= letterLittleZ - letterBigZ;

			i++;
		}

		return str;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::indexOf( TChar c ) const
	{
		return indexOf( c, 0 );
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::indexOf(TChar c, uint index) const
	{
		uint i = index;
		while (i < data->stringLength)
		{
			if (data->pData[i] == c) return i;
			i++;
		}
		return -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::indexOf( const BasicString<TChar>& str ) const
	{
		return indexOf( str, 0 );
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::indexOf(const BasicString<TChar>& str, uint index) const
	{
		if (str.data->stringLength > data->stringLength) return -1;

		uint i = index;
		uint c = data->stringLength - str.data->stringLength;

		do
		{
			uint b = 0;
			bool found = true;
			while (b < str.data->stringLength)
			{
				if (data->pData[i + b] != str.data->pData[b])
				{
					found = false;
					break;
				}
				b++;
			}

			if (found)
			{
				return i;
			}

			i++;
		}
		while (i <= c);

		return -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::lastIndexOf( TChar c ) const
	{
		return lastIndexOf( c, data->stringLength - 1u );
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::lastIndexOf( TChar c, uint index ) const
	{
		int i = index;
		while (i >= 0)
		{
			if ( data->pData[i] == c ) return i;
			i--;
		}

		return -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::lastIndexOf( const BasicString<TChar>& str ) const
	{
		return lastIndexOf( str, data->stringLength - str.data->stringLength );
	}

	template<typename TChar>
	TIKI_FORCE_INLINE int BasicString<TChar>::lastIndexOf( const BasicString<TChar>& str, uint index ) const
	{
		int i = index;
		while (i >= 0)
		{
			int b = 0;
			bool found = true;
			while (b < str.data->stringLength)
			{
				if (data->pData[i + b] != str.data->pData[b])
				{
					found = false;
					break;
				}
				b++;
			}

			if (found)
			{
				return i;
			}

			i--;
		}

		return -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::contains( TChar c ) const
	{
		return indexOf( c ) != -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::contains( const BasicString<TChar>& str ) const
	{
		return indexOf( str ) != -1;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::startsWith( TChar c ) const
	{
		if (data->stringLength < 1) return false;

		return data->pData[0] == c;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::startsWith(const BasicString<TChar>& str) const
	{
		if (data->stringLength < str.data->stringLength) return false;

		uint i = 0;
		while (i < str.data->stringLength)
		{
			if (data->pData[i] != str.data->pData[i]) return false;
			i++;
		}

		return true;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::endsWith(TChar c) const
	{
		if (data->stringLength < 1) return false;

		return data->pData[data->stringLength - 1] == c;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::endsWith(const BasicString<TChar>& str) const
	{
		if (data->stringLength < str.data->stringLength) return false;

		uint b = 0;
		uint i = data->stringLength - str.data->stringLength;
		while (i < data->stringLength)
		{
			if (data->pData[i] != str.data->pData[b]) return false;
			i++;
			b++;
		}

		return true;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE const TChar* BasicString<TChar>::operator*() const
	{
		return data->pData;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE TChar BasicString<TChar>::operator[](uint index) const
	{
		if (index >= data->stringLength) 
			throw "Index > Length";

		return data->pData[index];
	}

	template<typename TChar>
	TIKI_FORCE_INLINE TChar& BasicString<TChar>::operator[](uint index)
	{
		if (index >= data->stringLength)
			throw "Index > Length";

		if (data->refCount > 1)
		{
			StringRefData<TChar>* oldData = data;

			data = TIKI_NEW StringRefData<TChar>(
				data->stringLength,
				data->dataLength,
				data->pData
			);

			oldData->release();
		}

		return data->pData[index];
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator==(const BasicString<TChar>& rhs) const
	{
		if (data == rhs.data) return true;
		if (data->stringLength != rhs.data->stringLength) return false;

		uint i = 0;
		while (i < data->stringLength)
		{
			if (data->pData[i] != rhs.data->pData[i]) return false;
			i++;
		}

		return true;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator!=(const BasicString<TChar>& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar>& BasicString<TChar>::operator=(const BasicString<TChar>& rhs)
	{
		StringRefData<TChar>* oldData = data;

		data = rhs.data;
		data->addRef();

		oldData->release();

		return *this;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar> BasicString<TChar>::operator+(const BasicString<TChar>& rhs) const
	{
		uint len = data->stringLength + rhs.data->stringLength;
		BasicString<TChar> str = BasicString<TChar>(len);

		memory::copy(str.data->pData, data->pData, sizeof(TChar) * data->stringLength);
		memory::copy(str.data->pData + data->stringLength, rhs.data->pData, sizeof(TChar) * rhs.data->stringLength);
		str.data->pData[len] = 0;

		return str;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE BasicString<TChar>& BasicString<TChar>::operator+=(const BasicString<TChar>& rhs)
	{
		uint sl = data->stringLength;
		uint len = data->stringLength + rhs.data->stringLength;

		if (data->refCount != 1 || data->dataLength <= len)
		{
			StringRefData<TChar>* oldData = data;

			data = TIKI_NEW StringRefData<TChar>(
				len,
				calcLength(len + 1),
				oldData->pData,
				oldData->stringLength
			);

			oldData->release();
		}

		memory::copy(data->pData + sl, rhs.data->pData, sizeof(TChar) * rhs.data->stringLength);
		data->pData[len] = 0;
		data->stringLength = len;

		return *this;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator>(const BasicString<TChar>& rhs) const
	{
		if (data == rhs.data) return false;

		uint i = 0;
		uint c = (data->stringLength < rhs.data->stringLength ? data->stringLength : rhs.data->stringLength);
		while (i < c && data->pData[i] == rhs.data->pData[i])
		{
			i++;
		}

		return data->pData[i] > rhs.data->pData[i];
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator>=(const BasicString<TChar>& rhs) const
	{
		return (*this > rhs) || (*this == rhs);
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator<(const BasicString<TChar>& rhs) const
	{
		if (data == rhs.data) return false;

		uint i = 0;
		uint c = (data->stringLength < rhs.data->stringLength ? data->stringLength : rhs.data->stringLength);
		while (i < c && data->pData[i] == rhs.data->pData[i])
		{
			i++;
		}

		return data->pData[i] < rhs.data->pData[i];
	}

	template<typename TChar>
	TIKI_FORCE_INLINE bool BasicString<TChar>::operator<=(const BasicString<TChar>& rhs) const
	{
		return (*this < rhs) || (*this == rhs);
	}

	template<typename TChar>
	TIKI_FORCE_INLINE uint BasicString<TChar>::stringLength(const TChar* string) const
	{
		if (string == 0) return 0;

		uint c = 0;
		const TChar* str = string;
		while (*str != 0) { str++; c++; }

		return c;
	}

	template<typename TChar>
	TIKI_FORCE_INLINE uint BasicString<TChar>::calcLength( uint neededLen ) const
	{
		uint len = 2;
		while (len < neededLen) { len *= 2; }
		return len;
	}

	TIKI_FORCE_INLINE BasicString< char > operator+( const char* str1, const BasicString< char >& str2 )
	{
		return BasicString< char >(str1) + str2;
	}

	TIKI_FORCE_INLINE BasicString< wchar_t > operator+( const wchar_t* str1, const BasicString< wchar_t >& str2 )
	{
		return BasicString< wchar_t >(str1) + str2;
	}

	TIKI_FORCE_INLINE uint getStringLength( cstring pSource )
	{
		uint length = (uint)-1;		
		while ( pSource[ ++length ] != 0u );

		return length;
	}

	TIKI_FORCE_INLINE uint copyString( char* pTargetBuffer, uint bufferSize, cstring pSourceBuffer )
	{
		const uint sourceLength = TIKI_MIN( bufferSize - 1u, getStringLength( pSourceBuffer ) );

		uint64* pTarget64 = reinterpret_cast< uint64* >( pTargetBuffer );
		const uint64* pSource64 = reinterpret_cast< const uint64* >( pSourceBuffer );

		uint length = 0u;
		while ( length < sourceLength )
		{
			if ( ( sourceLength -  length ) >= sizeof( uint64 ) )
			{
				const uint lengthOver = length / sizeof( uint64 );
				pTarget64[ lengthOver ] = pSource64[ lengthOver ];

				length += sizeof( uint64 );
			}
			else
			{
				pTargetBuffer[ length ] = pSourceBuffer[ length ];
				++length;
			}
		}
		pTargetBuffer[ length ] = '\0';

		return length;
	}
}

#endif // TIKI_STRING_INL