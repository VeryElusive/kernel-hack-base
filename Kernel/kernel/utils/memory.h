#pragma once
#include "../sdk/windows/ntapi.h"
#include <intrin.h>

using uint64_t = LONGLONG;
using uint8_t = char;

//https://ntdiff.github.io/
#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180

#define PAGE_OFFSET_SIZE 12
#define PMASK ( ~0xfull << 8 ) & 0xfffffffffull

enum PROCTYPE {
	CLIENT,
	GAME
};

namespace Memory {
	PVOID GetProcessBaseAddress( PEPROCESS pProcess );

	DWORD GetUserDirectoryTableBaseOffset( );

	//check normal dirbase if 0 then get from UserDirectoryTableBas
	ULONG_PTR GetProcessCR3( PEPROCESS PROCTYPE );
	ULONG_PTR GetKernelDirBase( );

	NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written );

	NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead );

	//MmMapIoSpaceEx limit is page 4096 byte
	NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten );

	uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress );


	//
	NTSTATUS ReadProcessMemory( PROCTYPE proc, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteProcessMemory( PROCTYPE proc, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written );


	ULONG_PTR BruteForceDirectoryTableBase( HANDLE PID );

	inline ULONG_PTR CR3[ 2 ];

	__forceinline void memcpyINLINED( unsigned char* dest, const unsigned char* src, size_t size ) {
		for ( size_t i = 0; i < size; ++i ) {
			dest[ i ] = src[ i ];
		}
	}
}