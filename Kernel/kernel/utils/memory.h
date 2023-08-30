#pragma once
#include "../sdk/windows/ntapi.h"
#include "../communication/communication.h"

using uint64_t = LONGLONG;
using uint8_t = char;

namespace Memory {

	PVOID GetProcessBaseAddress( HANDLE pid );

	//https://ntdiff.github.io/
#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180

	DWORD GetUserDirectoryTableBaseOffset( );

	//check normal dirbase if 0 then get from UserDirectoryTableBas
	ULONG_PTR GetProcessCr3( PEPROCESS pProcess );
	ULONG_PTR GetKernelDirBase( );

	NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written );

	NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead );

	//MmMapIoSpaceEx limit is page 4096 byte
	NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten );

#define PAGE_OFFSET_SIZE 12
	static const uint64_t PMASK = ( ~0xfull << 8 ) & 0xfffffffffull;

	uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress );


	//
	NTSTATUS ReadProcessMemory( HANDLE pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteProcessMemory( HANDLE pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written );
}