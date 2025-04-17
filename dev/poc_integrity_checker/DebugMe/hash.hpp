#pragma once

#include <cstdint>
#include <string_view>

class Hash {
public:
	constexpr Hash( ) = default;
	constexpr ~Hash( ) = default;

	[[nodiscard]] static constexpr std::size_t fnv1a( std::string_view str ) noexcept {


		constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ull;
		constexpr std::uint64_t fnv_prime = 1099511628211ull;

		std::uint64_t hash = fnv_offset_basis;

		for ( auto c : str ) {
			hash ^= static_cast< std::size_t >( c );
			hash *= fnv_prime;
		}

		return hash;
	}
};

#define HASH(str) Hash::fnv1a(str)