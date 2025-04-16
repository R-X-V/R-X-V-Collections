#pragma once

#include "PebLdr.hpp"
#include "PortableExecutable.hpp"
#include "MemorySection.hpp"
#include "../Utils/GenericPtr.hpp"
#include "WinPlatform.hpp"

class WindowsDynamicLibrary {
public:
	explicit(true) WindowsDynamicLibrary( const char* libraryName )
		: handle { PebLdr{ WindowsPlatformApi::getPeb( )->ldr }.getModuleHandle( libraryName ) }
	{ }

	[[nodiscard]] explicit( true ) operator bool( ) const noexcept {
		return handle != nullptr;
	}

	[[nodiscard]] HMODULE getHandle( ) const noexcept {
		return handle;
	}

	[[nodiscard]] GenericPointer getFunctionAddress( std::string_view functionName ) const noexcept {

		if ( handle )
			return portableExecutable( ).getExport( functionName.data() );


		return {};
	}

	[[nodiscard]] MemorySection getCodeSection( ) const noexcept {
		if ( handle )
			return portableExecutable( ).getCodeSection( );

		return {};
	}

	[[nodiscard]] MemorySection getVmtSection( ) const noexcept {
		if ( handle )
			return portableExecutable( ).getVmtSection( );

		return {};
	}

	[[nodiscard]] MemorySection getDataSection( ) const noexcept {
		if ( handle )
			return portableExecutable( ).getDataSection( );

		return {};
	}



private:
	[[nodiscard]] PortableExecutable portableExecutable( ) const noexcept {

		return PortableExecutable { reinterpret_cast< const std::byte* >( handle ) };
	}

	HMODULE handle;
};