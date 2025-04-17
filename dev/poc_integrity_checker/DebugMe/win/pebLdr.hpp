#pragma once
#include "win.hpp"
#include "../utils/utils.hpp"

class PebLdr {
public:
	explicit(true) PebLdr( win::PebLdrData * ldr) : ldr(ldr ) { }

	[[nodiscard]] HMODULE getModuleHandle( const char* moduleName ) const noexcept {

		const auto head = getInLoadOrderModuleListHead( );

		for ( auto currentLink = head->Flink; currentLink != head; currentLink = currentLink->Flink ) {

			const auto& ldrEntry = *CONTAINING_RECORD( currentLink, win::LdrDataTableEntry, inLoadOrderLinks );

			if ( utils::equalsCaseInsensitive( ldrEntry.baseDllName, moduleName ))
			{
				return HMODULE(ldrEntry.dllBase);
			}
		}

		return { };
	}

private:
	[[nodiscard]] PLIST_ENTRY getInLoadOrderModuleListHead( ) const noexcept {

		return &ldr->inLoadOrderModuleList;
	}


private:
	win::PebLdrData* ldr;
};