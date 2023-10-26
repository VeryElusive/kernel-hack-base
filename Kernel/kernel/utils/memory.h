#pragma once
#include "../sdk/windows/ntapi.h"

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

namespace Memory {
	__forceinline PVOID GetProcessBaseAddress( PEPROCESS pProcess );

	__forceinline DWORD GetUserDirectoryTableBaseOffset( );

	//check normal dirbase if 0 then get from UserDirectoryTableBas
	__forceinline ULONG_PTR GetProcessCr3( PEPROCESS pProcess );
	__forceinline ULONG_PTR GetKernelDirBase( );

	__forceinline NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read );

	__forceinline NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written );

	__forceinline NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead );

	//MmMapIoSpaceEx limit is page 4096 byte
	__forceinline NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten );

	__forceinline uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress );


	//
	__forceinline NTSTATUS ReadProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read );

	__forceinline NTSTATUS WriteProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written );

	__forceinline void memcpyINLINED( unsigned char* dest, const unsigned char* src, size_t size ) {
		for ( size_t i = 0; i < size; ++i ) {
			dest[ i ] = src[ i ];
		}
	}

	__forceinline PVOID GetProcessBaseAddress( PEPROCESS pProcess ) {
		PVOID Base = PsGetProcessSectionBaseAddress( pProcess );
		ObDereferenceObject( pProcess );
		return Base;
	}

	__forceinline DWORD GetUserDirectoryTableBaseOffset( )
	{
		RTL_OSVERSIONINFOW ver = { 0 };
		RtlGetVersion( &ver );

		switch ( ver.dwBuildNumber )
		{
		case WINDOWS_1803:
			return 0x0278;
			break;
		case WINDOWS_1809:
			return 0x0278;
			break;
		case WINDOWS_1903:
			return 0x0280;
			break;
		case WINDOWS_1909:
			return 0x0280;
			break;
		case WINDOWS_2004:
			return 0x0388;
			break;
		case WINDOWS_20H2:
			return 0x0388;
			break;
		case WINDOWS_21H1:
			return 0x0388;
			break;
		default:
			return 0x0388;
		}
	}

	//check normal dirbase if 0 then get from UserDirectoryTableBas
	__forceinline ULONG_PTR GetProcessCr3( PEPROCESS pProcess )
	{
		PUCHAR process = ( PUCHAR ) pProcess;
		ULONG_PTR process_dirbase = *( PULONG_PTR ) ( process + 0x28 ); //dirbase x64, 32bit is 0x18
		if ( process_dirbase == 0 )
		{
			DWORD UserDirOffset = GetUserDirectoryTableBaseOffset( );
			ULONG_PTR process_userdirbase = *( PULONG_PTR ) ( process + UserDirOffset );
			return process_userdirbase;
		}
		return process_dirbase;
	}
	__forceinline ULONG_PTR GetKernelDirBase( )
	{
		PUCHAR process = ( PUCHAR ) PsGetCurrentProcess( );
		ULONG_PTR cr3 = *( PULONG_PTR ) ( process + 0x28 ); //dirbase x64, 32bit is 0x18
		return cr3;
	}

	__forceinline NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read )
	{
		uint64_t paddress = TranslateLinearAddress( dirbase, address );
		return ReadPhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, read );
	}

	__forceinline NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written )
	{
		uint64_t paddress = TranslateLinearAddress( dirbase, address );
		return WritePhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, written );
	}

	__forceinline NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead )
	{
		MM_COPY_ADDRESS AddrToRead = { 0 };
		AddrToRead.PhysicalAddress.QuadPart = reinterpret_cast< LONGLONG >( TargetAddress );
		return MmCopyMemory( lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead );
	}

	//MmMapIoSpaceEx limit is page 4096 byte
	__forceinline NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten )
	{
		if ( !TargetAddress )
			return STATUS_UNSUCCESSFUL;

		PHYSICAL_ADDRESS AddrToWrite = { 0 };
		AddrToWrite.QuadPart = reinterpret_cast< LONGLONG >( TargetAddress );

		PVOID pmapped_mem = MmMapIoSpaceEx( AddrToWrite, Size, PAGE_READWRITE );

		if ( !pmapped_mem )
			return STATUS_UNSUCCESSFUL;

		//memcpy( pmapped_mem, lpBuffer, Size );

		memcpyINLINED( ( unsigned char* ) pmapped_mem, ( unsigned char* ) lpBuffer, Size );

		*BytesWritten = Size;
		MmUnmapIoSpace( pmapped_mem, Size );
		return STATUS_SUCCESS;
	}

	__forceinline uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress ) {
		directoryTableBase &= ~0xf;

		uint64_t pageOffset = virtualAddress & ~( ~0ul << PAGE_OFFSET_SIZE );
		uint64_t pte = ( ( virtualAddress >> 12 ) & ( 0x1ffll ) );
		uint64_t pt = ( ( virtualAddress >> 21 ) & ( 0x1ffll ) );
		uint64_t pd = ( ( virtualAddress >> 30 ) & ( 0x1ffll ) );
		uint64_t pdp = ( ( virtualAddress >> 39 ) & ( 0x1ffll ) );

		SIZE_T readsize = 0;
		uint64_t pdpe = 0;
		ReadPhysicalAddress( reinterpret_cast< PVOID >( directoryTableBase + 8 * pdp ), &pdpe, sizeof( pdpe ), &readsize );
		if ( ~pdpe & 1 )
			return 0;

		uint64_t pde = 0;
		ReadPhysicalAddress( reinterpret_cast< PVOID >( ( pdpe & PMASK ) + 8 * pd ), &pde, sizeof( pde ), &readsize );
		if ( ~pde & 1 )
			return 0;

		/* 1GB large page, use pde's 12-34 bits */
		if ( pde & 0x80 )
			return ( pde & ( ~0ull << 42 >> 12 ) ) + ( virtualAddress & ~( ~0ull << 30 ) );

		uint64_t pteAddr = 0;
		ReadPhysicalAddress( reinterpret_cast< PVOID >( ( pde & PMASK ) + 8 * pt ), &pteAddr, sizeof( pteAddr ), &readsize );
		if ( ~pteAddr & 1 )
			return 0;

		/* 2MB large page */
		if ( pteAddr & 0x80 )
			return ( pteAddr & PMASK ) + ( virtualAddress & ~( ~0ull << 21 ) );

		virtualAddress = 0;
		ReadPhysicalAddress( reinterpret_cast< PVOID >( ( pteAddr & PMASK ) + 8 * pte ), &virtualAddress, sizeof( virtualAddress ), &readsize );
		virtualAddress &= PMASK;

		if ( !virtualAddress )
			return 0;

		return virtualAddress + pageOffset;
	}


	//
	__forceinline NTSTATUS ReadProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read )
	{
		NTSTATUS NtRet{ STATUS_SUCCESS };

		ULONG_PTR process_dirbase = GetProcessCr3( pProcess );
		ObDereferenceObject( pProcess );

		SIZE_T CurOffset = 0;
		uint64_t TotalSize = size;
		while ( TotalSize )
		{

			uint64_t CurPhysAddr = TranslateLinearAddress( process_dirbase, ( ULONG64 ) Address + CurOffset );
			if ( !CurPhysAddr ) return STATUS_UNSUCCESSFUL;

			ULONG64 ReadSize = min( PAGE_SIZE - ( CurPhysAddr & 0xFFF ), TotalSize );
			SIZE_T BytesRead = 0;
			NtRet = ReadPhysicalAddress( reinterpret_cast< PVOID >( CurPhysAddr ), ( PVOID ) ( ( ULONG64 ) AllocatedBuffer + CurOffset ), ReadSize, &BytesRead );
			TotalSize -= BytesRead;
			CurOffset += BytesRead;
			if ( NtRet != STATUS_SUCCESS ) break;
			if ( BytesRead == 0 ) break;
		}

		*read = CurOffset;
		return NtRet;
	}

	__forceinline NTSTATUS WriteProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written )
	{
		NTSTATUS NtRet{ STATUS_SUCCESS };

		ULONG_PTR process_dirbase = GetProcessCr3( pProcess );
		ObDereferenceObject( pProcess );

		SIZE_T CurOffset = 0;
		uint64_t TotalSize = size;
		while ( TotalSize )
		{
			uint64_t CurPhysAddr = TranslateLinearAddress( process_dirbase, ( ULONG64 ) Address + CurOffset );
			if ( !CurPhysAddr ) return STATUS_UNSUCCESSFUL;

			ULONG64 WriteSize = min( PAGE_SIZE - ( CurPhysAddr & 0xFFF ), TotalSize );
			SIZE_T BytesWritten = 0;
			NtRet = WritePhysicalAddress( reinterpret_cast< PVOID >( CurPhysAddr ), ( PVOID ) ( ( ULONG64 ) AllocatedBuffer + CurOffset ), WriteSize, &BytesWritten );
			TotalSize -= BytesWritten;
			CurOffset += BytesWritten;
			if ( NtRet != STATUS_SUCCESS ) break;
			if ( BytesWritten == 0 ) break;
		}

		*written = CurOffset;
		return NtRet;
	}
}