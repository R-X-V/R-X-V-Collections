#pragma once

#include "pebLdr.hpp"
#include "portableExecutable.hpp"
#include "memorySection.hpp"
#include "../utils/GenericPtr.hpp"
#include "winPlatform.hpp"

class WindowsModule {
public:
	explicit(true) WindowsModule( const char* libraryName )
		: handle { PebLdr{ WindowsPlatformApi::getPeb( )->ldr }.getModuleHandle( libraryName ) }
	{ }
	explicit( true ) WindowsModule( void* base )
		: handle {reinterpret_cast<HMODULE>( base) } { }

	[[nodiscard]] explicit( true ) operator bool( ) const noexcept {
		return handle != nullptr;
	}

	[[nodiscard]] HMODULE getHandle( ) const noexcept {
		return handle;
	}

	[[nodiscard]] PortableExecutable getPortableExecutable( ) const noexcept {

		return PortableExecutable { reinterpret_cast< const std::byte* >( handle ) };
	}

private:


	HMODULE handle;
};