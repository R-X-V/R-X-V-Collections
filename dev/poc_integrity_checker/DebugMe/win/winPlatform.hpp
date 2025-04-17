#pragma once

#include "win.hpp"
#include <bit>
#include <intrin.h>

class WindowsPlatformApi {
public:
    [[nodiscard]] static win::Peb* getPeb( ) noexcept {

    #if defined(_M_X64) || defined(__x86_64__) || defined(_WIN64)
        return std::bit_cast< win::Peb* >( __readgsqword( 0x60 ) ); // x64: GS:[0x60]
    #else
        return std::bit_cast< Win::Peb* >( __readfsdword( 0x30 ) ); // x86: FS:[0x30]
    #endif
    }
         
    static void debugBreak( ) noexcept {
        __debugbreak( );
    }
};
