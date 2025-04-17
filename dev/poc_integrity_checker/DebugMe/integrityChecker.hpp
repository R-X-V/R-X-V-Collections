#pragma once
#include "win/memorySection.hpp"
#include "win/winModule.hpp"
#include "hash.hpp"

// Can be later expanded to check any non-writable sections
// Can be expanded to check all functions using unwind data .pdata section 

class IntegrityChecker {
public:
	[[nodiscard]] bool checkTextSection( const WindowsModule& mod, const WindowsModule& modCopy ) const noexcept {

		if ( !mod.getHandle( ) || !modCopy.getHandle( ) )
			return false;

		
		auto codeSection = mod.getPortableExecutable( ).getCodeSection( );
		auto codeSectionCopy = modCopy.getPortableExecutable( ).getCodeSection( );

		return checkSection( codeSection, codeSectionCopy);
	}

	// 
#if defined(_M_X64)
	// Checks the integrity of a given function by comparing its bytes in memory "mod" to the expected, trusted version from a verified source "modCopy"
	// "funcTrustedAddr" is the trusted runtime address within "modCopy"
	[[nodiscard]] bool checkFunction( const WindowsModule& mod, const WindowsModule& modCopy, std::uintptr_t funcTrustedAddr ) {

		auto pe = mod.getPortableExecutable( );

		auto codeSectionCopy = modCopy.getPortableExecutable( ).getCodeSection( );

		if ( !codeSectionCopy.contains( funcTrustedAddr ) )
			return false;

		UNWIND_HISTORY_TABLE table;
		std::uintptr_t base;

		const std::uintptr_t fnAddr = pe.getBase( ) + codeSectionCopy.baseOffsetTo( funcTrustedAddr );

		const auto runFuncEntry = RtlLookupFunctionEntry( fnAddr, &base, &table );

		if ( !runFuncEntry )
			return false;

		const std::size_t funcSize = runFuncEntry->EndAddress - runFuncEntry->BeginAddress;
		
		return Hash::fnv1a( std::string_view( reinterpret_cast< const char* >( fnAddr ), funcSize ) )
			== Hash::fnv1a( std::string_view( reinterpret_cast< const char* >( funcTrustedAddr ), funcSize ) );

	}
#endif

private:
	[[nodiscard]] bool checkSection(const MemorySection& section, const MemorySection& sectionCopy ) const noexcept {

		const auto& raw = section.raw( );
		const auto& rawCopy = sectionCopy.raw( );

		if ( raw.data() == nullptr || rawCopy.data() == nullptr || !raw.size( ) || !rawCopy.size( ) || raw.size( ) != rawCopy.size( ))
			return false;

		return std::memcmp( raw.data( ), rawCopy.data( ), raw.size( ) ) == 0;
	}

	[[nodiscard]] std::size_t hashraw( std::span<const std::byte> source ) const noexcept {

		return HASH( std::string_view(  reinterpret_cast<const char*>(source.data( )), source.size( ) ) );
	}



};