#pragma once
#include "../win/win.hpp"

namespace utils
{
	[[nodiscard]] constexpr wchar_t toUpper( wchar_t c ) {

		if ( c >= L'a' && c <= L'z' )
			return c - 0x20;

		return c;
	}

	[[nodiscard]] inline unsigned short unicodeLengthInChars( const UNICODE_STRING& str ) noexcept {
		return str.Length / sizeof( wchar_t );
	}

	[[nodiscard]] bool equalsCaseInsensitive( const UNICODE_STRING& str, const char* otherString ) noexcept {
		for ( std::size_t i = 0; i < unicodeLengthInChars( str ); ++i )
		{
			if ( toUpper( str.Buffer[i] ) != toUpper( otherString[i] ) || otherString[i] == '\0' )
				return false;
		}
		return true;
	}


}