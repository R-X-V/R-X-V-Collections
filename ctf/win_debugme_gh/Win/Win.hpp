#pragma once

#include "../Utils/Pad.hpp"
#include "../Utils/Utils.hpp"

namespace Win
{

	struct ListEntry {
		ListEntry* fLink;
		ListEntry* bLink;
	};
#pragma once

	struct PebLdrData {
#if defined(_M_X64) || defined(__x86_64__) || defined(_WIN64)
		PAD( 0x10 );
#else
		PAD( 0x0C );
#endif
		ListEntry inLoadOrderModuleList;
		ListEntry inMemoryOrderModuleList;
		ListEntry inInitializationOrderModuleList;
	};

	struct Peb {
		std::byte _pad0[0x02];
		bool beingDebugged;
		std::byte _pad1[0x04];
		void* imageBaseAddress;
		PebLdrData* ldr;
		std::byte _pad2[0x58];
		int ntGlobalFlag;
	};

	struct UnicodeString {
		unsigned short length;
		unsigned short maximumLength;
		wchar_t* buffer;

		[[nodiscard]] unsigned short lengthInChars( ) const noexcept {
			return length / sizeof( wchar_t );
		}

		[[nodiscard]] bool equalsCaseInsensitive( const char* otherString ) const noexcept {
			for ( std::size_t i = 0; i < lengthInChars( ); ++i ) {
				if ( utils::toUpper( buffer[i] ) != utils::toUpper( otherString[i] ) || otherString[i] == '\0' )
					return false;
			}
			return true;
		}
	};


	struct LdrDataTableEntry {
		ListEntry inLoadOrderLinks;
		ListEntry inMemoryOrderLinks;
		ListEntry inInitializationOrderLinks;
		void* dllBase;
		void* entryPoint;
		unsigned long sizeOfImage;
		UnicodeString fullDllName;
		UnicodeString baseDllName;
	};

}