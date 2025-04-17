#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>

#include "loader.hpp"
#include "IntegrityChecker.hpp"

int main( ) {

	Loader loader;
	IntegrityChecker checker;
	WindowsModule ntdll { "ntdll.dll" }; 	// Not trusted 

	// Trusted load 
	if ( !loader.loadImageFromMemory<HASH( "ntdll.dll" )>( ) )
		return 0;
		
	WindowsModule trustedNtdll { loader.baseAddress }; 
	auto trustedFunc = trustedNtdll.getPortableExecutable( ).getExport( "NtQueryInformationProcess" ).getRawAddress();

	if ( !trustedFunc )
		return 0;

	while ( !GetAsyncKeyState( VK_END ) & 1 ) {

		
		bool integrity = checker.checkFunction( ntdll, trustedNtdll, trustedFunc ); 

		BOOL present {};

		auto success = CheckRemoteDebuggerPresent( GetCurrentProcess( ), &present );

		if ( !success || present || !integrity) {
			std::cout << "debugger detected ! " << std::endl;
			break;
		}
	
		std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	}

	return 0;
}