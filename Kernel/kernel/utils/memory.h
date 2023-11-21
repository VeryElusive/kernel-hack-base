#pragma once
#include "../sdk/windows/ntapi.h"
#include "../utils/utils.h"
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

typedef struct _KPRCB* PKPRCB;

typedef struct _KAFFINITY_EX
{
	USHORT Count;
	USHORT Size;
	ULONG Reserved;
	ULONGLONG Bitmap[ 20 ];
} KAFFINITY_EX, * PKAFFINITY_EX;

/// <summary>
/// Structure that we'll pass to our NMI to provide & receive back data.
/// </summary>
typedef struct _NMI_CALLBACK_DATA {
	PEPROCESS target_process;
	uintptr_t cr3;
} NMI_CALLBACK_DATA, * PNMI_CALLBACK_DATA;

extern "C" __declspec( dllimport ) VOID NTAPI KeInitializeAffinityEx( PKAFFINITY_EX affinity );
extern "C" __declspec( dllimport ) VOID NTAPI KeAddProcessorAffinityEx( PKAFFINITY_EX affinity, INT num );
extern "C" __declspec( dllimport ) VOID NTAPI HalSendNMI( PKAFFINITY_EX affinity );
extern "C" __declspec( dllimport ) PKPRCB NTAPI KeQueryPrcbAddress( __in ULONG Number );

namespace Memory {
	PVOID GetProcessBaseAddress( PEPROCESS pProcess );

	DWORD GetUserDirectoryTableBaseOffset( );

	//check normal dirbase if 0 then get from UserDirectoryTableBas
	ULONG_PTR GetProcessCr3( PEPROCESS pProcess );
	ULONG_PTR GetKernelDirBase( );

	NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written );

	NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead );

	//MmMapIoSpaceEx limit is page 4096 byte
	NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten );

	uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress );


	//
	NTSTATUS ReadProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read );

	NTSTATUS WriteProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written );

	/// <summary>
	/// Sleep the currently executing thread.
	/// </summary>
	/// <param name="milliseconds">Number of milliseconds to sleep.</param>
	/// <returns>void</returns>
	NTSTATUS Sleep( LONG milliseconds )
	{
		INT64 interval = milliseconds * -10000i64;
		return KeDelayExecutionThread( KernelMode, FALSE, ( PLARGE_INTEGER ) &interval );
	}

	/// <summary>
	/// Given a active core index, fetch the currently executing thread and then
	/// subsuqently it's process and return that.
	/// </summary>
	/// <param name="core_index">Which core do we want to fetch the process for?</param>
	/// <returns>PEPROCESS running on the core or NULL</returns>
	__forceinline PEPROCESS GetProcessExecutingOnCore( USHORT core_index ) {
		//Get a pointer to this core's KPRCB.
		PKPRCB pkprcb = KeQueryPrcbAddress( core_index );
		if ( !pkprcb )
			return NULL;

		//Get the currently executing thread.
		uintptr_t current_thread = *( uintptr_t* ) ( ( uintptr_t ) pkprcb + 0x8 );
		if ( !current_thread )
			return NULL;

		//Get the process for this currently executing thread.
		return PsGetThreadProcess( ( PETHREAD ) current_thread );
	}

	/// <summary>
	/// Simple NMI callback routine.
	/// </summary>
	/// <param name="ctx"></param>
	/// <param name="handled"></param>
	/// <returns></returns>
	BOOLEAN callback( PVOID ctx, BOOLEAN handled )
	{
		//Cast the supplied context to a pointer for our callback data.
		PNMI_CALLBACK_DATA nmi_data = ( PNMI_CALLBACK_DATA ) ctx;

		//Check if the current process that's executing is still the target process?
		if ( nmi_data->target_process == IoGetCurrentProcess( ) ) {

			//Target process still being executed, read the CR3 and put it into our data structure.
			nmi_data->cr3 = __readcr3( );
		}
		return TRUE;
	}

	/// <summary>
	/// Using NMIs and the KPRCB to find the game's CR3 value while it's unencrypted.
	/// </summary>
	/// <param name="process">Target process to get the CR3 value for</param>
	/// <param name="timeout">How many milliseconds until this function fails if no CR3 is found</param>
	/// <returns></returns>
	uintptr_t GetProcessCr3ByNMIs( PEPROCESS process, LONG timeout = 1000 ) {
		//Get the current time that we started.
		LARGE_INTEGER curr_time;
		KeQuerySystemTime( &curr_time );

		//Get the time to stop trying
		LONGLONG EndTryingTime = curr_time.QuadPart + ( timeout * 10000 );

		//Setup structure to pass as context for our NMI.
		_NMI_CALLBACK_DATA nmi_data = {};
		nmi_data.target_process = process;

		//Loop until our alloted time runs out.
		while ( curr_time.QuadPart < EndTryingTime ) {

			//Get the number of active processors.
			ULONG processor_count = KeQueryActiveProcessorCountEx( ALL_PROCESSOR_GROUPS );

			//Iterate each active core
			for ( USHORT i = 0; i < processor_count; i++ ) {

				//Get the process currently executing on this core.
				PEPROCESS executing_proc = GetProcessExecutingOnCore( i );

				//This core is currently executing a thread in the process we're interested in.
				if ( executing_proc == process ) {

					//Immediately fire our own NMI.
					PVOID nmi_handle = KeRegisterNmiCallback( callback, &nmi_data );
					KAFFINITY_EX affinity;
					KeInitializeAffinityEx( &affinity );
					KeAddProcessorAffinityEx( &affinity, i );
					HalSendNMI( &affinity );

					//Sleep for a small bit.
					Sleep( 2000 );

					//Unregister the NMI.
					KeDeregisterNmiCallback( nmi_handle );

					//Handle result (assuming it completed within the time we slept).
					if ( nmi_data.cr3 && ( nmi_data.cr3 >> 0x38 ) != 0x40 )
						return nmi_data.cr3;

					//Reset the cr3 value in the nmi data struct since it was invalid.
					nmi_data.cr3 = NULL;
				}
			}

			//Update loop time.
			KeQuerySystemTime( &curr_time );
		}

		return NULL;
	}

	void memcpyINLINED( unsigned char* dest, const unsigned char* src, size_t size ) {
		for ( size_t i = 0; i < size; ++i ) {
			dest[ i ] = src[ i ];
		}
	}

	PVOID GetProcessBaseAddress( PEPROCESS pProcess ) {
		if ( !pProcess )
			return 0;

		return PsGetProcessSectionBaseAddress( pProcess );
	}

	DWORD GetUserDirectoryTableBaseOffset( )
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

	BOOLEAN IsDirectoryBaseEncrypted( _In_ ULONGLONG DirectoryBase ) {
		BOOLEAN Encrypted = ( DirectoryBase >> 0x38 ) == 0x40;
		return Encrypted;
	}

	//check normal dirbase if 0 then get from UserDirectoryTableBas, then if it is encrypted, we must wait until it gets decrypted
	ULONG_PTR GetProcessCr3( PEPROCESS pProcess )
	{
		PUCHAR process = ( PUCHAR ) pProcess;
		ULONG_PTR process_dirbase = *( PULONG_PTR ) ( process + 0x28 ); //dirbase x64, 32bit is 0x18
		if ( process_dirbase == 0 ) {
			DWORD UserDirOffset = GetUserDirectoryTableBaseOffset( );
			process_dirbase = *( PULONG_PTR ) ( process + UserDirOffset );
		}

		if ( IsDirectoryBaseEncrypted( process_dirbase ) ) 
			process_dirbase = GetProcessCr3ByNMIs( pProcess );

		return process_dirbase;
	}
	ULONG_PTR GetKernelDirBase( )
	{
		PUCHAR process = ( PUCHAR ) PsGetCurrentProcess( );
		ULONG_PTR cr3 = *( PULONG_PTR ) ( process + 0x28 ); //dirbase x64, 32bit is 0x18
		return cr3;
	}

	NTSTATUS ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read )
	{
		uint64_t paddress = TranslateLinearAddress( dirbase, address );
		return ReadPhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, read );
	}

	NTSTATUS WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written )
	{
		uint64_t paddress = TranslateLinearAddress( dirbase, address );
		return WritePhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, written );
	}

	NTSTATUS ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead )
	{
		MM_COPY_ADDRESS AddrToRead = { 0 };
		AddrToRead.PhysicalAddress.QuadPart = reinterpret_cast< LONGLONG >( TargetAddress );
		return MmCopyMemory( lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead );
	}

	//MmMapIoSpaceEx limit is page 4096 byte
	NTSTATUS WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten )
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

	uint64_t TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress ) {
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
	NTSTATUS ReadProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read )
	{
		if ( !pProcess )
			return STATUS_UNSUCCESSFUL;

		NTSTATUS NtRet = STATUS_SUCCESS;

		ULONG_PTR process_dirbase = GetProcessCr3( pProcess );

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

	NTSTATUS WriteProcessMemory( PEPROCESS pProcess, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written )
	{
		if ( !pProcess )
			return STATUS_UNSUCCESSFUL;

		NTSTATUS NtRet = STATUS_SUCCESS;

		ULONG_PTR process_dirbase = GetProcessCr3( pProcess );
		

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