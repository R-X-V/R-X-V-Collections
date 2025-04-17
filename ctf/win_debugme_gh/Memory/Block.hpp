#include <Windows.h>
#include <cstddef>

#pragma once

static constexpr std::size_t PAGE_SIZE { 0x1000 };

class Block {
public:
	[[nodiscard]] static Block& getBlock( ) {
		static Block instance;
		return instance;
	}

	[[nodiscard]] void* allocate( std::size_t allocSize )  noexcept {

		if ( used + allocSize > capacity )
			return nullptr;

		if ( !base ) {
			base = reinterpret_cast< std::byte* >(
				VirtualAlloc( nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE )
				);

			assert( base && "Block::allocate() : VirtualAlloc failed" );

			nextFree = base;
		}


		void* addr = nextFree;

		capacity -= allocSize;
		used += allocSize;
		nextFree += allocSize;

		return addr;
	}

private:
	Block( ) :
		size { PAGE_SIZE },
		capacity { PAGE_SIZE },
		base { nullptr },
		nextFree { base },
		used { 0 } { }

	~Block( ) {
		if ( base ) {
			VirtualFree( base, 0, MEM_RELEASE );
			base = nullptr;
			nextFree = nullptr;
		}
	}

	// prevent copying
	Block( const Block& ) = delete;
	Block& operator=( const Block& ) = delete;

private:
	std::byte* base;
	std::byte* nextFree;
	std::size_t size;
	std::size_t capacity;
	std::size_t used;

};