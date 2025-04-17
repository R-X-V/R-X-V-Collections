#pragma once

#include <cstdint>
#include <array>
#include <cassert>
#include <bit>

#include "PageProtector.hpp"
#include "../Utils/GenericPtr.hpp"
#include "Block.hpp"


/*
Minimalist hook utility
*/

class Hook {
public:
	constexpr Hook( ) = default;
	constexpr ~Hook( ) = default;

	template <typename T1, typename T2>
	constexpr explicit( true ) Hook( T1 target, T2 detour, std::size_t opcodeLen ) requires ( AddressLike<T1> && AddressLike<T2> ) :
		target { toAddress( target ) }, detour { toAddress( detour ) }, opcodeLen { opcodeLen } 
	{
	}

	[[nodiscard]] bool install( ) {

		if ( !target || !detour || active || installed || opcodeLen < hookSize )
			return false;

		PageProtector guard( target, hookSize );

		original.resize( opcodeLen );

		for ( std::size_t i = 0; i < opcodeLen; i++ )
			original[i] = target[i];

		trampoline = reinterpret_cast< std::byte* >( Block::getBlock( ).allocate( hookSize * 2 + opcodeLen ) );

		if ( !trampoline )
			return false;

		// [trampoline] -> detour
		writeJmp( trampoline, detour );

		// [trampoline+5] -> original instructions
		// std::memcpy( trampoline + hookSize, original.data( ), opcodeLen );
		for ( std::size_t i = 0; i < opcodeLen; i++ )
			trampoline[i + hookSize] = original[i];

		// [trampoline+5+opcodeLen] -> jump back to target+opcodeLen
		writeJmp( trampoline + hookSize + opcodeLen, target + opcodeLen );

		FlushInstructionCache( GetCurrentProcess( ), target, hookSize );

		installed = true;

		return true;
	}

	template <typename T1, typename T2>
	void registerTarget( T1 _target, T2 _detour, std::size_t _opcodeLen ) noexcept {

		target = toAddress(_target);
		detour = toAddress(_detour);
		opcodeLen = _opcodeLen;
	}


	bool enable( ) noexcept {

		if ( !target || active || !installed )
			return false;

		PageProtector guard( target, hookSize );

		// [target] -> trampoline
		writeJmp( target, trampoline );

		FlushInstructionCache( GetCurrentProcess( ), target, opcodeLen );

		active = true;

		return true;

	}

	bool disable( ) noexcept {

		if ( !target || !active || !installed )
			return false;

		PageProtector guard( target, hookSize );

		for ( std::size_t i = 0; i < opcodeLen; i++ ) 
			target[i] = original[i];
		

		FlushInstructionCache( GetCurrentProcess( ), target, opcodeLen );

		active = false;

		return true;
	}

	template <typename Fn, class ... Args>
	auto __stdcall callOriginal( Args&&... args ) {
		return reinterpret_cast< Fn >( getHkOriginal( ) )( std::forward<Args>( args )... );
	}


	[[nodiscard]] void* getHkOriginal( ) const noexcept {
		return trampoline + hookSize;
	}

	[[nodiscard]] void* getTrampoline( ) const noexcept {
		return trampoline;
	}

	[[nodiscard]] bool isActive( ) const noexcept {
		return active;
	}

	[[nodiscard]] bool isInstalled( ) const noexcept {
		return installed;
	}
private:
	void writeJmp( void* src, void* dst ) {

		auto rel = static_cast< std::ptrdiff_t >( reinterpret_cast< std::uintptr_t >( dst )
												  - reinterpret_cast< std::uintptr_t >( src )
												  - hookSize
												  );

		auto srcByte = reinterpret_cast< std::byte* >( src );

		srcByte[0] = std::byte( 0xE9 );
		*reinterpret_cast< std::int32_t* >( &srcByte[1] ) = rel;
	}


private:

	std::byte* target = nullptr;
	std::byte* detour = nullptr;
	std::byte* trampoline = nullptr;
	std::size_t opcodeLen = 0;
	bool active = false;
	bool installed = false;

	static constexpr std::size_t hookSize { 5 }; // JMP rel32 is 5 bytes
	std::vector<std::byte> original {};
};