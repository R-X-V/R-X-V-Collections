#pragma once

#include <winternl.h>

#pragma comment(lib, "ntdll")


namespace win
{


	extern "C" NTSYSCALLAPI NTSTATUS NtCreateSection( _Out_ PHANDLE SectionHandle,
													  _In_ ACCESS_MASK DesiredAccess,
													  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
													  _In_opt_ PLARGE_INTEGER MaximumSize,
													  _In_ ULONG SectionPageProtection,
													  _In_ ULONG AllocationAttributes,
													  _In_opt_ HANDLE FileHandle );

	extern "C" NTSYSCALLAPI NTSTATUS NtOpenSection( _Out_ PHANDLE SectionHandle,
													_In_ ACCESS_MASK DesiredAccess,
													_In_ POBJECT_ATTRIBUTES ObjectAttributes );


	extern "C" NTSYSCALLAPI NTSTATUS NtMapViewOfSection( _In_ HANDLE SectionHandle,
														 _In_ HANDLE ProcessHandle,
														 _Inout_ PVOID * BaseAddress,
														 _In_ ULONG_PTR ZeroBits,
														 _In_ SIZE_T CommitSize,
														 _Inout_opt_ PLARGE_INTEGER SectionOffset,
														 _Inout_ PSIZE_T ViewSize,
														 _In_ ULONG InheritDisposition,
														 _In_ ULONG AllocationType,
														 _In_ ULONG Win32Protect );

	extern "C" NTSYSCALLAPI NTSTATUS NtUnmapViewOfSection( _In_ HANDLE ProcessHandle,
														   _In_ PVOID  BaseAddress );


	const enum _SECTION_INHERIT {

		ViewShare = 1,
		ViewUnmap = 2
	};

#pragma once

	struct PebLdrData {
		std::int32_t length;
		BYTE initialized;
		PVOID ssHandle;
		LIST_ENTRY inLoadOrderModuleList;
		LIST_ENTRY inMemoryOrderModuleList;
		LIST_ENTRY inInitializationOrderModuleList;
	};

	struct Peb {
		std::uint8_t inheritedAddressSpace;
		std::uint8_t readImageFileExecOptions;
		bool beingDebugged;
		std::uint8_t bitField;
		std::int32_t _padding0;
		PVOID mutant;
		PVOID imageBaseAddress;
		PebLdrData* ldr;
	#if defined(_WIN64)
		std::byte _padding1[0x9C];

	#else
		std::byte _padding1[0x58];

	#endif
		std::int32_t ntGlobalFlag;
	};


	struct LdrDataTableEntry {
		LIST_ENTRY inLoadOrderLinks;
		LIST_ENTRY inMemoryOrderLinks;
		LIST_ENTRY inInitializationOrderLinks;
		PVOID dllBase;
		PVOID entryPoint;
		unsigned long sizeOfImage;
		UNICODE_STRING fullDllName;
		UNICODE_STRING baseDllName;
	};

}