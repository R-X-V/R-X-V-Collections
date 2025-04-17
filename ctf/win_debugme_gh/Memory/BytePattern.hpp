#pragma once
#include <optional>
#include <string_view>
#include <span>
#include <cassert>

class BytePattern {
public:
	constexpr explicit( true ) BytePattern( std::string_view pattern, std::optional<char> wildcardChar = { std::nullopt } )
		: pattern{pattern }, wildcardChar { wildcardChar }
	{ }

	template <typename T>
	[[nodiscard]] static BytePattern ofObject( const T& object ) noexcept {
		
		return BytePattern { std::string_view{reinterpret_cast< const char* >( &object ), sizeof( T )} };
	}

	[[nodiscard]] inline bool matches( std::span<const std::byte> bytes ) const noexcept {

		assert( bytes.size( ) == pattern.size( ) );

		for ( std::size_t i = 0; i < bytes.size( ); ++i ) {

			if ( std::to_integer<char>( bytes[i] ) != pattern[i] && pattern[i] != wildcardChar )
				return false;
		}
		return true;
	}

	[[nodiscard]] std::size_t getLength( ) const noexcept {

		return pattern.length( );
	}

	[[nodiscard]] std::size_t getIndexOfFirstNonWildcardChar( ) {

		if ( wildcardChar.has_value( ) )
			return pattern.find_first_not_of( wildcardChar.value( ) );

		return 0;
	}

	[[nodiscard]] std::size_t getIndexOfLastNonWildcardChar( ) {

		if ( wildcardChar.has_value( ) )
			return pattern.find_last_not_of( wildcardChar.value( ) );

		return pattern.size( ) - 1;
	}

	[[nodiscard]] char getBack( ) const noexcept {
		return pattern.back( );
	}

	[[nodiscard]] char getFront( ) const noexcept {
		return pattern.front( );
	}

	[[nodiscard]] std::string_view getRaw( ) const noexcept {
		return pattern;
	}

	[[nodiscard]] std::optional<char> getWildcardChar( ) const noexcept {
		return wildcardChar;
	}

private:
	std::string_view pattern;
	std::optional<char> wildcardChar;
};