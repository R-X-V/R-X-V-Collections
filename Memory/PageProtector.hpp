#pragma once
#include <Windows.h>
#include <cstddef>

class PageProtector {
public:
	PageProtector( void* addr, std::size_t len ) noexcept {
		this->addr = addr;
		this->len = len;
		VirtualProtect( addr, len, PAGE_EXECUTE_READWRITE, &oldProtect );
	}

	~PageProtector( ) noexcept {
		DWORD _;
		VirtualProtect( addr, len, oldProtect, &_ );
	}

private:
	void* addr;
	std::size_t len;
	DWORD oldProtect;
};