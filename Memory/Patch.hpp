#pragma once
#include <cstdint>
#include <array>
#include <cassert>
#include <bit>

#include "PageProtector.hpp"
#include "../Utils/GenericPtr.hpp"


template <typename T>
concept AddressLike = std::is_pointer_v<T> || std::is_integral_v<T> || std::is_same_v<T, GenericPointer>;


template <typename T>
static std::byte* toAddress( T ptr ) noexcept {

	if constexpr ( std::is_pointer_v<T> || std::is_integral_v<T> ) 
		return std::bit_cast< std::byte* >( ptr );
	else if constexpr ( std::is_same_v<T, GenericPointer> ) 
		return reinterpret_cast< std::byte* >( ptr.GetRawAddress( ) );
	else 
		static_assert( sizeof( T ) == 0, "Unsupported type in toAddress()" );
}

class IPatch {
public:
	virtual ~IPatch( ) = default;

	virtual void apply( ) noexcept = 0;

	virtual void restore( ) noexcept = 0;
};


template <std::size_t size>
class Patch : public IPatch {
public:

	template <typename T>
	explicit( true ) Patch( T target ) noexcept requires( size > 0 && AddressLike<T> )
		: address { toAddress( target ) }, enabled { false } {
		save( );
	}

	
	constexpr explicit( true ) Patch( ) noexcept requires( size > 0)
		: address { nullptr }, enabled { false }
	{}

	~Patch( ) {
		if ( enabled ) 
			restore( );
	}

	template <typename T>
	void initialize( T target, std::initializer_list<std::uint8_t> bytes ) noexcept  {

		assert( !enabled && "Patch already initialized" );
		assert( bytes.size( ) <= size && "Too many elements for patch size" );

		address = toAddress( target );

		std::size_t i = 0;

		for ( auto b : bytes ) 
			patchedBytes[i++] = static_cast< std::byte >( b );
		

		save( );
	}

	// Apply a new patch with an integer value
	template <typename T>
	void apply( T value ) noexcept requires (std::is_integral_v<T>) {

		PageProtector guard( address, size );

		const std::byte* valueBytes = reinterpret_cast< const std::byte* >( &value );

		for ( std::size_t i = 0; i < sizeof( T ) && i < size; ++i ) {
			address[i] = valueBytes[i];
			patchedBytes[i] = valueBytes[i];
		}

		enabled = true;
	}

	// Apply the most recent patch again
	void apply( ) noexcept override {
		
		PageProtector guard( address, size );

		for ( std::size_t i = 0; i < size; ++i )
			address[i] = patchedBytes[i];

		enabled = true;
	}

	// Restore original bytes
	void restore( ) noexcept override {
		
		if ( !enabled ) 
			return;

		PageProtector guard( address, size );

		for ( std::size_t i = 0; i < size; ++i )
			address[i] = originalBytes[i];

		enabled = false;
	}

private:
	void save( ) noexcept {
		for ( std::size_t i = 0; i < size; ++i )
			originalBytes[i] = address[i];
	}

private:
	std::byte* address;
	std::array<std::byte, size> originalBytes {};
	std::array<std::byte, size> patchedBytes {};
	bool enabled;
};