#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <windows.h>

#include "hash.hpp"
#include "win/win.hpp"
#include "win/winModule.hpp"


/*
  Simple loader that currently support :
  - Loading image from disk using native NTapis
  - Loading image from memory using native NTapis 
*/

class Loader {
public:
	Loader( ) = default;
	~Loader( ) {

		if( baseAddress )
			win::NtUnmapViewOfSection( GetCurrentProcess( ), baseAddress );
	}

	//  Loads a file from disk into memory using standard file I/O
	[[deprecated( "This function is not usable yet. Do not call it." )]]
	[[nodiscard]] bool loadFromDisk( const std::wstring_view filePath ) noexcept {

		// Ensure the file exists and is not a directory
		if ( !std::filesystem::exists( filePath ) || std::filesystem::is_directory( filePath ) )
			return false;

		// Open the file and seek to the end to determine size
		std::ifstream file( filePath.data( ), std::ios::binary | std::ios::ate );

		if ( !file.is_open( ) )
			return false;

		const std::streamsize size = file.tellg( );

		if ( size <= 0 )
			return false;

		// Go to the start
		file.seekg( 0, std::ios::beg );

		buffer.resize( size );

		if ( !file.read( reinterpret_cast< char* >( buffer.data( ) ), size ) )
			return false;

		baseAddress = reinterpret_cast< LPVOID >( buffer.data( ) );

		return true;
	}

	// Loads and maps the file as a PE image using native NT APIs
	[[nodiscard]] bool loadImageFromDisk( const std::wstring_view filePath ) noexcept {

		// Open file handle with read access
		HANDLE fileHandle = CreateFileW( filePath.data( ), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

		if ( fileHandle == INVALID_HANDLE_VALUE )
			return false;

		HANDLE sectionHandle { nullptr };

		// Create a section object with SEC_IMAGE to map it like a loaded PE
		NTSTATUS status = win::NtCreateSection( &sectionHandle,
											GENERIC_READ,
											nullptr,
											nullptr,
											PAGE_READONLY,
											SEC_IMAGE,
											fileHandle );

		if ( !NT_SUCCESS( status ) )
			return false;

		std::size_t viewSize { };

		// Map the section into current process's memory as read-only
		status = win::NtMapViewOfSection( sectionHandle,
									 GetCurrentProcess( ),
									 &baseAddress,
									 NULL,
									 NULL,
									 nullptr,
									 &viewSize,
									 win::ViewUnmap,
									 NULL,
									 PAGE_READONLY );


		CloseHandle( sectionHandle );


		if ( !NT_SUCCESS( status ) || !baseAddress )
			return false;

		return true;
	}

	// Loads global memory mapped file (_SECTION) using section name resolved from hash
	template <std::size_t hash>
	[[nodiscard]] bool loadImageFromMemory(  ) noexcept {

		UNICODE_STRING sectionName {};

		OBJECT_ATTRIBUTES attributes {};

		// Initalize Object attributes depending on the given module hash
		switch ( hash )
		{
		case HASH( "ntdll.dll" ):
			RtlInitUnicodeString( &sectionName, L"\\KnownDlls\\ntdll.dll" );
			break;

		case HASH( "kernel32.dll" ):
			RtlInitUnicodeString( &sectionName, L"\\KnownDlls\\kernel32.dll" );
			break;

		default:
			return false;
		}

		InitializeObjectAttributes( &attributes, &sectionName, OBJ_CASE_INSENSITIVE, nullptr, nullptr );


		HANDLE sectionHandle { nullptr };

		// Open the existing a section object with SEC_IMAGE
		NTSTATUS status = win::NtOpenSection( &sectionHandle, SECTION_MAP_READ, &attributes );

		if ( !NT_SUCCESS( status ) )
			return false;

		std::size_t viewSize { };

		// Map the section into current process's memory as read-only
		status = win::NtMapViewOfSection(
			sectionHandle,
			GetCurrentProcess( ),
			&baseAddress,
			NULL,
			NULL,
			nullptr,
			&viewSize,
			win::ViewUnmap,
			NULL,
			PAGE_READONLY
		);

		CloseHandle( sectionHandle );

		if ( !NT_SUCCESS( status ) || !baseAddress )
			return false;

		return true;

	}

	[[nodiscard]] WindowsModule getModule( ) noexcept {

		return WindowsModule { reinterpret_cast<HMODULE>( baseAddress) };
	}
public:
	PVOID baseAddress { nullptr };
	std::vector<std::byte> buffer;

};