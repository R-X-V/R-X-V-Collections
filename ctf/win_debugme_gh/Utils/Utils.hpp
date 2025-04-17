#pragma once

namespace utils
{
	[[nodiscard]] constexpr wchar_t toUpper( wchar_t c ) {

		if ( c >= L'a' && c <= L'z' )
			return c - 0x20;

		return c;
	}
}