#pragma once
#include "Memory/BytePattern.hpp"
#include "Memory/Patch.hpp"
#include "Memory/Hook.hpp"
#include "TlHelp32.h"
#include <winternl.h>

namespace sehBypass
{

	namespace forceNoDbg
	{
		constexpr BytePattern pattern { std::string_view( "\xC6\x05\x00\x00\x00\x00\x01\xFF\x15", 9 ), '\x00' };
		constexpr std::ptrdiff_t offset { 6 };

		constexpr std::initializer_list<std::uint8_t> bytes = { 0x00 };
		constinit Patch<1> patch;

	}

	namespace forceNoBrkp
	{
		constexpr BytePattern pattern { std::string_view( "\xCC\xA0", 2 ) };
		constexpr std::ptrdiff_t offset { 0 };

		constexpr std::initializer_list<std::uint8_t> bytes = { 0x90 };
		constinit Patch<1> patch;
	}

} 

namespace checkRemoteDebuggerPresentBypass
{
	using fn_t = bool( __stdcall* ) (HANDLE hProcess,  PBOOL pbDebuggerPresent  );
	constexpr std::string_view fnName { "CheckRemoteDebuggerPresent" };
	constexpr std::size_t len { 5 };
	Hook hook;
	
	static __declspec( noinline ) bool detour( HANDLE hProcess, PBOOL pbDebuggerPresent ) {

		if ( hProcess == GetCurrentProcess( ) ) {

			return false;
		}

		return hook.callOriginal< fn_t>( hProcess, pbDebuggerPresent );
	}
}

namespace checkParentProcessBypass
{
	using fn_t = BOOL( __stdcall* ) ( HANDLE hSnapshot, LPPROCESSENTRY32W lppe  );
	constexpr std::string_view fnName { "Process32NextW" };
	constexpr std::size_t len { 5 };
	Hook hook;

	static __declspec( noinline ) BOOL __stdcall detour( HANDLE hSnapshot, LPPROCESSENTRY32W lppe ) {

		
		auto result = hook.callOriginal< fn_t>( hSnapshot, lppe );

		if ( lppe->th32ProcessID == GetCurrentProcessId( ) )
			lppe->th32ParentProcessID = lppe->th32ProcessID; // we overwrite the parent process id, with any value (as long as it not our debugger)

		
		return result;
	}
}

namespace hiddenThreadBypass
{
	using fnSet_t = NTSTATUS( __stdcall* ) ( HANDLE threadHandle, THREADINFOCLASS threadInformationClass, PVOID threadInformation, ULONG threadInformationLength );
	constexpr std::string_view fnName { "NtSetInformationThread" };
	constexpr std::size_t len { 5 };
	Hook hook;


	static __declspec( noinline ) NTSTATUS __stdcall detour( HANDLE threadHandle, THREADINFOCLASS threadInformationClass, PVOID threadInformation, ULONG threadInformationLength ) {

	
		if ( threadInformationClass != 0x11 )
			return hook.callOriginal< fnSet_t>( threadHandle, threadInformationClass, threadInformation, threadInformationLength );


		return STATUS_INVALID_PARAMETER;
	}

	[[nodiscard]] bool isThreadHidden( WindowsDynamicLibrary& ntdll, HANDLE hThread ) noexcept {

		auto ntQueryInformationThreadAddr = ntdll.getFunctionAddress( "NtQueryInformationThread" );

		using fnQuery_t = NTSTATUS( __stdcall* ) ( HANDLE threadHandle, THREADINFOCLASS threadInformationClass, PVOID threadInformation, ULONG threadInformationLength, PULONG returnLength );

		bool hidden { false };
		ULONG len;

		auto status = ntQueryInformationThreadAddr.as<fnQuery_t>( )( hThread, static_cast< THREADINFOCLASS >( 0x11 ), &hidden, 1, &len );


		return NT_SUCCESS( status ) ? hidden : false;
	}


	[[nodiscard]] HANDLE getHiddenThread( WindowsDynamicLibrary& ntdll ) noexcept {

		DWORD pid = GetCurrentProcessId( );
		DWORD mainThreadId = 0;

		HANDLE hiddenThread = INVALID_HANDLE_VALUE;
		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if ( snapshot == INVALID_HANDLE_VALUE )
			return 0;


		THREADENTRY32 te = { sizeof( te ) };
		if ( Thread32First( snapshot, &te ) ) {
			do {
				if ( te.th32OwnerProcessID == pid ) {
					// Open the thread to query its creation time
					HANDLE hThread = OpenThread( THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID );
					if ( hThread ) {
						
						if ( isThreadHidden(ntdll, hThread) ) 							{
							hiddenThread = hThread;
							CloseHandle( hThread );
							break;
						}

						CloseHandle( hThread );
					}
				}
			} while ( Thread32Next( snapshot, &te ) );
		}

		CloseHandle( snapshot );

		return hiddenThread;
	}

}