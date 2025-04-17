
#include "Win/PortableExecutable.hpp"
#include "Win/WinDLL.hpp"
#include "Memory/Patch.hpp"
#include "Bypass.hpp"
#include "Config.hpp"
#include "Memory/Hook.hpp"
#include <iostream>


/*

	Small bypass for GH::debugme with c++20 minimalist library

	Warning : Could be optimized many ways !
	- First make it safer by checking returned values !
	- replace std::memcpy when possible
	- Apply proper logging and actions on failure
*/


static void InitConsole( FILE* file ) {

	if ( AllocConsole( ) ) {
		freopen_s( &file, "CONIN$", "r", stdin );
		freopen_s( &file, "CONOUT$", "w", stdout );
		freopen_s( &file, "CONOUT$", "w", stderr );
	}
}

static void CloseConsole( FILE* file ) {

	if ( file ) {
		fclose( file );
		file = nullptr;
	}

	FreeConsole( );
}

FILE* file { nullptr };

using FnAdd_t = bool( __stdcall ) ( const int a, const int b );

bool checkParentProcess( ) {

	std::cout << "function hooked" << std::endl;

	return false;

}

DWORD WINAPI EntryPoint( LPVOID hModule ) {

	InitConsole( file );


	PortableExecutable antiDebug;
	WindowsDynamicLibrary kernelbase { "kernelbase.dll" };
	WindowsDynamicLibrary kernel32 { "kernel32.dll" };
	WindowsDynamicLibrary ntdll { "ntdll.dll" };

	assert( antiDebug.getBase( ) );
	assert( kernelbase.getHandle( ) );
	assert( kernel32.getHandle( ) );
	assert( ntdll.getHandle( ) );

	std::cout << "[*] base address :" << std::hex << antiDebug.getBase( ).GetRawAddress( )
		<< "\n[*] kernelbase address :" << kernelbase.getHandle( )
		<< "\n[*] kernel32 address :" << kernel32.getHandle( )
		<< "\n[*] ntdll address :" << ntdll.getHandle( ) << "\n\n";

	auto& bypass = BypassGH::GetBypass( );

	/* Initialize Refererences */
	Patch<1>& sehForceNoDbg = sehBypass::forceNoDbg::patch;
	Patch<1>& sehForceNoBrkp = sehBypass::forceNoBrkp::patch;
	Hook& checkRemoteDebuggerPresentHk = checkRemoteDebuggerPresentBypass::hook;
	Hook& checkParentProcessHk = checkParentProcessBypass::hook;
	Hook& hiddenThreadHk = hiddenThreadBypass::hook;


	/* Search memory addresses */
	auto sehForceNoDbgAddr = antiDebug.searchInCodeSection( sehBypass::forceNoDbg::pattern ) + sehBypass::forceNoDbg::offset;
	auto sehForceNoBrkpAddr = antiDebug.searchInCodeSection( sehBypass::forceNoBrkp::pattern ) + sehBypass::forceNoBrkp::offset;
	auto checkRemoteDebuggerPresentAddr = kernelbase.getFunctionAddress( checkRemoteDebuggerPresentBypass::fnName );
	auto process32NextAddr = kernel32.getFunctionAddress( checkParentProcessBypass::fnName );
	auto ntSetInfoThreadAddr = ntdll.getFunctionAddress( hiddenThreadBypass::fnName );

	assert( sehForceNoDbgAddr );
	assert( sehForceNoBrkpAddr );
	assert( checkRemoteDebuggerPresentAddr );
	assert( process32NextAddr );
	assert( ntSetInfoThreadAddr );

	/* Log*/
	std::cout << "[*] locate anti-debugged.exe:UnhandledExecptionFilter1 :" << std::hex << sehForceNoDbgAddr.GetRawAddress( ) 
		<< "\n[*] locate anti-debugged.exe:UnhandledExecptionFilter2 :"  << sehForceNoBrkpAddr.GetRawAddress( ) 
		<< "\n[*] locate kernelbase::CheckRemoteDebuggerPresent :" << checkRemoteDebuggerPresentAddr.GetRawAddress( ) 
		<< "\n[*] locate kernel32::Process32NextW :" << checkRemoteDebuggerPresentAddr.GetRawAddress( ) 
		<< "\n[*] locate ntdll::NtSetInformationThread :" << checkRemoteDebuggerPresentAddr.GetRawAddress( ) << "\n\n";

	/* Initalize Patches */
	sehForceNoDbg.initialize( sehForceNoDbgAddr, sehBypass::forceNoDbg::bytes );
	sehForceNoBrkp.initialize( sehForceNoBrkpAddr, sehBypass::forceNoBrkp::bytes );

	/* Initalize Hooks */
	checkRemoteDebuggerPresentHk.registerTarget(
		checkRemoteDebuggerPresentAddr,
		checkRemoteDebuggerPresentBypass::detour,
		checkRemoteDebuggerPresentBypass::len
	);

	checkParentProcessHk.registerTarget(
		process32NextAddr,
		checkParentProcessBypass::detour,
		checkParentProcessBypass::len
	);

	hiddenThreadHk.registerTarget(
		ntSetInfoThreadAddr,
		hiddenThreadBypass::detour,
		hiddenThreadBypass::len
	);

	/* Add Patches */
	bypass.addPatch( std::ref( sehForceNoDbg ) );
	bypass.addPatch( std::ref( sehForceNoBrkp ) );

	/* Add Hooks */
	bypass.addHook( std::ref( checkRemoteDebuggerPresentHk ) );
	bypass.addHook( std::ref( checkParentProcessHk ) );
	bypass.addHook( std::ref( hiddenThreadHk ) );

	bypass.applyAll( );

	std::cout << "[*] Patching UnhandledExecptionFilter : done\n";
	std::cout << "[*] Hooking CheckRemoteDebuggerPresent : done" << "\n[*] Hooking Process32NextW : done" << "\n[*] Hooking NtSetInformationThread : done\n";

	antiDebug.deleteDebugFlags( );
	std::cout << "[*] Overwriting debug flags in PEB :  done\n";

	antiDebug.deleteWow64DebugFlags( );
	std::cout << "[*] Overwriting debug flags in PEB (WOW64) : done\n";

	/* Log Hidden threads if any */
	
	if ( auto hThread = hiddenThreadBypass::getHiddenThread( ntdll ); hThread != INVALID_HANDLE_VALUE ) {
		std::cout << "[*] Warning hidden thread detected : " << hThread << '\n';
	}

	std::cout << "\n\n[**] anti-debugging fully bypassed" << std::endl;


	return 0;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, LPVOID reserved ) {


	switch ( reason ) {
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls( instance );
			EntryPoint( instance ); // for safer execution or for debugging, create a thread
			break;
		}

		case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

		case DLL_THREAD_DETACH:
		break;

		case DLL_PROCESS_DETACH:
		CloseConsole( file ); // work done 
		auto& bypass = BypassGH::GetBypass( );
		bypass.restoreAll( );
		break;
	}

	return TRUE;
}