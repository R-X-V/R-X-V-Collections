#pragma once
#include <cstddef>
#include <span>
#include <cassert>

class MemorySection {
public:
	MemorySection( ) = default;
	explicit(true) MemorySection( std::span<const std::byte> section ) noexcept
		: base { reinterpret_cast< std::uintptr_t >( section.data() ) }
		, size {section.size() } 
	{ }

	[[nodiscard]] std::span<const std::byte> raw( ) const noexcept {

		return { reinterpret_cast<const std::byte* >( base ), size };
	}

	[[nodiscard]] auto contains( std::uintptr_t address, std::size_t objectSize ) const noexcept {

		return address >= base && size >= objectSize && ( address - base ) <= size - objectSize;
 	}

	[[nodiscard]] auto contains( std::uintptr_t address ) const noexcept {

		return address >= base && address - base < size;
	}

	[[nodiscard]] auto contains( std::uint32_t offset ) const noexcept {

		return  (base + offset) - base < size;
	}

	[[nodiscard]] auto offsetTo( std::uintptr_t address ) const noexcept {
		assert( contains( address ) );

		return address - base;
	}

	[[nodiscard]] auto baseOffsetTo( std::uintptr_t address ) const noexcept {
		assert( contains( address ) );

		return address - (base & ~0xFFFF);
	}


private:
	std::uintptr_t base { };
	std::size_t size { };
};