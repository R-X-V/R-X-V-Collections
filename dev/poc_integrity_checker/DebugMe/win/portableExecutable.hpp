#pragma once
#include <Windows.h>
#include <array>

#include "MemorySection.hpp"
#include "WinPlatform.hpp"
#include "../memory/bytePattern.hpp"
#include "../memory/block.hpp"
#include "../utils/genericPtr.hpp"




class PortableExecutable {
public:


	explicit( true ) PortableExecutable(  ) noexcept
		: base { reinterpret_cast<const std::byte*>(WindowsPlatformApi::getPeb()->imageBaseAddress) } {
		assert( base );
	}

	template <typename T>
	explicit PortableExecutable( T addr ) noexcept
		requires ( std::is_pointer_v<T> || std::is_integral_v<T> )
	: base { reinterpret_cast< const std::byte* >( addr ) } { }


	[[nodiscard]] GenericPointer getBase( ) const noexcept {
		return { base };
	}

	[[nodiscard]] MemorySection getCodeSection( ) const noexcept {
		
		for ( const auto& section : getSectionHeaders( ) ) {

			if( (section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE) ) != 0 && std::memcmp(section.Name, ".text", 5 ) == 0 )
				return MemorySection { std::span{ base + section.VirtualAddress, section.Misc.VirtualSize } };
		}

		return { };
	}

	[[nodiscard]] MemorySection getVmtSection( ) const noexcept {

		for ( const auto& section : getSectionHeaders( ) ) {

			if ( ( section.Characteristics & IMAGE_SCN_MEM_READ ) != 0 && std::memcmp( section.Name, ".rdata", 6 ) == 0 )
				return MemorySection { std::span{ base + section.VirtualAddress, section.Misc.VirtualSize } };
		}

		return {};
	}

	[[nodiscard]] MemorySection getDataSection( ) const noexcept {

		for ( const auto& section : getSectionHeaders( ) ) {

			if ( ( section.Characteristics & IMAGE_SCN_MEM_READ ) != 0 && std::memcmp( section.Name, ".data", 5 ) == 0 )
				return MemorySection { std::span{ base + section.VirtualAddress, section.Misc.VirtualSize } };
		}

		return {};
	}

	[[nodiscard]] GenericPointer getExport( const char* name ) const noexcept {

		const auto exportDataDir = getExportDataDirectory( );

		if ( !exportDataDir )
			return { };

		const auto exportDirectory = reinterpret_cast< const IMAGE_EXPORT_DIRECTORY* >( base + exportDataDir->VirtualAddress );

		for ( DWORD i = 0; i < exportDirectory->NumberOfNames; ++i ) {

			const auto exportName = reinterpret_cast< const char* >( base + reinterpret_cast< const DWORD* >( base + exportDirectory->AddressOfNames )[i] );

			if ( std::strcmp( exportName, name ) == 0 ) {

				const auto nameOrdinals = reinterpret_cast< const WORD* >( base + exportDirectory->AddressOfNameOrdinals );
				const auto functions = reinterpret_cast< const DWORD* >( base + exportDirectory->AddressOfFunctions );
				const auto functionRva = functions[nameOrdinals[i]];

				return base + functionRva;
			}
		};

		return {};
	}

	[[nodiscard]] void deleteDebugFlags(  ) const noexcept {

		WindowsPlatformApi::getPeb( )->beingDebugged = 0;
		WindowsPlatformApi::getPeb( )->ntGlobalFlag = 0;
	}

	[[nodiscard]] void deleteWow64DebugFlags( ) const noexcept {

		constexpr std::array<std::uint8_t,43> heavenGatePatchPeb64 = {
			0x6A, 0x33,                         // push 0x33
			0xE8, 0x00, 0x00, 0x00, 0x00,       // call $+5
			0x83, 0x04, 0x24, 0x05,             // add dword ptr [esp], 5
			0xCB,                               // retf

			0x65, 0x67, 0x48, 0xA1, 0x60, 0x00, 0x00, 0x00, // mov rax, gs:[0x60]
			0xC6, 0x40, 0x02, 0x00,             // mov byte ptr [rax+2], 0
			0xE8, 0x00, 0x00, 0x00, 0x00,       // call $+5
			0xC7, 0x44, 0x24, 0x04, 0x23, 0x00, 0x00, 0x00, // mov dword ptr [rsp+4], 0x23
			0x83, 0x04, 0x24, 0x0D,             // add dword ptr [rsp], 0x0D
			0xCB,                                // retf
			0xC3                                 // ret
		};
		constexpr std::size_t patchSize { heavenGatePatchPeb64.size( ) };

		std::byte* gate = reinterpret_cast<std::byte*>(Block::getBlock( ).allocate( patchSize ));

		for ( std::size_t i = 0; i < patchSize; i++ ) {
			gate[i] = std::bit_cast<std::byte>(heavenGatePatchPeb64[i]);
		}

		using fn_t = void( __stdcall* )( );
		reinterpret_cast< void( __stdcall* )( ) >( gate )( );

		return;

	}

	[[nodiscard]] GenericPointer searchInCodeSection( const BytePattern& pattern ) {

		const auto codeSection = getCodeSection( ).raw( );
		const auto patternLen = pattern.getLength( );

		for ( std::size_t i = 0; i <= codeSection.size( ) - patternLen; i++ ) {

			const auto currPtr = codeSection.data( ) + i;

			std::span<const std::byte> candidate {
				reinterpret_cast< const std::byte* >( currPtr ),
				patternLen
			};

			if ( pattern.matches( candidate ) )
				return { currPtr };

		}

		return { };
	}


	// pure utility function
	[[nodiscard]] static bool isForwardedExport( DWORD functionRva, const IMAGE_DATA_DIRECTORY& exportDataDirectory ) noexcept {

		return functionRva >= exportDataDirectory.VirtualAddress && functionRva - exportDataDirectory.VirtualAddress < exportDataDirectory.Size; // Is functionRva within the export directory memory range ?

	}

	[[nodiscard]] std::span<const IMAGE_SECTION_HEADER> getSectionHeaders( ) const noexcept {

		if ( const auto ntHeaders = getNtHeaders( ); ntHeaders )
			return { 
			reinterpret_cast< const IMAGE_SECTION_HEADER* >( reinterpret_cast< const std::byte* >( &ntHeaders->OptionalHeader ) + ntHeaders->FileHeader.SizeOfOptionalHeader )  ,
			ntHeaders->FileHeader.NumberOfSections };

		return {};

	}

	[[nodiscard]] const IMAGE_SECTION_HEADER* getSectionByName( const BYTE name[IMAGE_SIZEOF_SHORT_NAME]) const noexcept {

		for ( const auto& section : getSectionHeaders( ) )
		{
			if ( std::memcmp(name, section.Name, IMAGE_SIZEOF_SHORT_NAME ) == 0 )
				return &section;
		}

		return nullptr;
	}

	[[nodiscard]] const IMAGE_DATA_DIRECTORY* getExportDataDirectory( ) const noexcept {
		return getDataDirectory( IMAGE_DIRECTORY_ENTRY_EXPORT );
	}

	[[nodiscard]] const IMAGE_OPTIONAL_HEADER* getOptionalHeader( ) const noexcept {

		if ( const auto ntHeaders = getNtHeaders( ); ntHeaders && ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC )
			return &ntHeaders->OptionalHeader;

		return nullptr;
	}

	[[nodiscard]] const IMAGE_DATA_DIRECTORY* getDataDirectory( std::uint8_t entry ) const noexcept {
	
		if ( const auto ntHeader = getNtHeaders( ); ntHeader && ntHeader->OptionalHeader.NumberOfRvaAndSizes > entry )
			return &ntHeader->OptionalHeader.DataDirectory[entry];

		return nullptr;
	}


	[[nodiscard]] const IMAGE_NT_HEADERS* getNtHeaders( ) const noexcept {

		if ( const auto dosHeader = getDosHeader( ) )
			return getNtHeaders( *dosHeader );

		return nullptr;

	}

	[[nodiscard]] const IMAGE_NT_HEADERS* getNtHeaders( const IMAGE_DOS_HEADER& dosHeader ) const noexcept {


		if ( const auto ntHeaders = reinterpret_cast< const IMAGE_NT_HEADERS* >( base + dosHeader.e_lfanew ); ntHeaders->Signature == IMAGE_NT_SIGNATURE )
			return ntHeaders;

		return nullptr;

	}

	[[nodiscard]] const IMAGE_DOS_HEADER* getDosHeader( ) const noexcept {

		if ( const auto dosHeader = reinterpret_cast< const IMAGE_DOS_HEADER* >( base ); dosHeader->e_magic == IMAGE_DOS_SIGNATURE )
			return dosHeader;
		
		return nullptr;
	}

private:
	const std::byte* base;
};
