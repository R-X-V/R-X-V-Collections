#pragma once
#include "Windows.h"
#include "Win.hpp"


class PebLdr {
public:
	explicit(true) PebLdr( Win::PebLdrData * ldr) : ldr(ldr ) { }

	[[nodiscard]] HMODULE getModuleHandle( const char* moduleName ) const noexcept {

		const auto head = getInLoadOrderModuleListHead( );

		for ( auto currentLink = head->fLink; currentLink != head; currentLink = currentLink->fLink ) {

			const auto& ldrEntry = *CONTAINING_RECORD( currentLink, Win::LdrDataTableEntry, inLoadOrderLinks );

			if ( ldrEntry.baseDllName.equalsCaseInsensitive( moduleName ) ) {
				return HMODULE(ldrEntry.dllBase);
			}
		}

		return { };
	}

	// Get Current base addresss


private:
	[[nodiscard]] Win::ListEntry* getInLoadOrderModuleListHead( ) const noexcept {

		return &ldr->inLoadOrderModuleList;
	}


private:
	Win::PebLdrData* ldr;
};