#include "memory.h"
#include "../utils/utils.h"


PVOID Memory::GetProcessBaseAddress( PEPROCESS pProcess ) {
	if ( !pProcess )
		return 0;

	return PsGetProcessSectionBaseAddress( pProcess );
}

ULONG_PTR Memory::BruteForceDirectoryTableBase( HANDLE PID ) {
	const auto proc{ Utils::LookupPEProcessFromID( PID ) };
	if ( !proc )
		return 0;

	auto base_address = PsGetProcessSectionBaseAddress( proc );
	if ( !base_address )
		return 0;

	VirtAddr_t virtual_address{ };
	virtual_address.value = base_address;
	SIZE_T read{ };

	auto ranges = MmGetPhysicalMemoryRanges( );

	for ( int i = 0;; i++ ) {
		auto elem = &ranges[ i ];

		if ( !elem->BaseAddress.QuadPart || !elem->NumberOfBytes.QuadPart )
			return 0;

		ULONG_PTR current_physical = elem->BaseAddress.QuadPart;

		for ( int j = 0; j < ( elem->NumberOfBytes.QuadPart / 0x1000 ); j++, current_physical += 0x1000 ) {
			_MMPTE pml4e = { 0 };
			ReadPhysicalAddress( reinterpret_cast< void* >( current_physical + 8 * virtual_address.pml4_index ), &pml4e, 8, &read );
			if ( !pml4e.u.Hard.Valid )
				continue;

			_MMPTE pdpte = { 0 };
			ReadPhysicalAddress( reinterpret_cast< void* >( ( pml4e.u.Hard.PageFrameNumber << 12 ) + 8 * virtual_address.pdpt_index ), &pdpte, 8, &read );
			if ( !pdpte.u.Hard.Valid )
				continue;

			_MMPTE pde = { 0 };
			ReadPhysicalAddress( reinterpret_cast< void* >( ( pdpte.u.Hard.PageFrameNumber << 12 ) + 8 * virtual_address.pd_index ), &pde, 8, &read );
			if ( !pde.u.Hard.Valid )
				continue;

			_MMPTE pte = { 0 };
			ReadPhysicalAddress( reinterpret_cast< void* >( ( pde.u.Hard.PageFrameNumber << 12 ) + 8 * virtual_address.pt_index ), &pte, 8, &read );
			if ( !pte.u.Hard.Valid )
				continue;

			uint64_t physical_base = TranslateLinearAddress( current_physical, reinterpret_cast< uint64_t >( base_address ) );
			if ( !physical_base )
				continue;

			char buffer[ sizeof( IMAGE_DOS_HEADER ) ];
			ReadPhysicalAddress( reinterpret_cast< void* >( physical_base ), buffer, sizeof( IMAGE_DOS_HEADER ), &read );

			PIMAGE_DOS_HEADER header = reinterpret_cast< PIMAGE_DOS_HEADER >( buffer );
			if ( header->e_magic != IMAGE_DOS_SIGNATURE )
				continue;

			// from debugging its always -2 ?
			return current_physical + 2u;
		}
	}

}

DWORD Memory::GetUserDirectoryTableBaseOffset( ) {
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

ULONG_PTR Memory::GetKernelDirBase( )
{
	PUCHAR process = ( PUCHAR ) PsGetCurrentProcess( );
	ULONG_PTR cr3 = *( PULONG_PTR ) ( process + 0x28 ); //dirbase x64, 32bit is 0x18
	return cr3;
}

NTSTATUS Memory::ReadVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* read )
{
	uint64_t paddress = TranslateLinearAddress( dirbase, address );
	return ReadPhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, read );
}

NTSTATUS Memory::WriteVirtual( uint64_t dirbase, uint64_t address, uint8_t* buffer, SIZE_T size, SIZE_T* written )
{
	uint64_t paddress = TranslateLinearAddress( dirbase, address );
	return WritePhysicalAddress( reinterpret_cast< PVOID >( paddress ), buffer, size, written );
}

NTSTATUS Memory::ReadPhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead )
{
	MM_COPY_ADDRESS AddrToRead = { 0 };
	AddrToRead.PhysicalAddress.QuadPart = reinterpret_cast< LONGLONG >( TargetAddress );
	return MmCopyMemory( lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead );
}

//MmMapIoSpaceEx limit is page 4096 byte
NTSTATUS Memory::WritePhysicalAddress( PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten )
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

uint64_t Memory::TranslateLinearAddress( uint64_t directoryTableBase, uint64_t virtualAddress ) {
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
NTSTATUS Memory::ReadProcessMemory( PROCTYPE proc, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read )
{
	NTSTATUS NtRet = STATUS_SUCCESS;

	ULONG_PTR process_dirbase = CR3[ proc ];
	if ( !process_dirbase )
		return STATUS_UNSUCCESSFUL;

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

NTSTATUS Memory::WriteProcessMemory( PROCTYPE proc, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written )
{
	NTSTATUS NtRet = STATUS_SUCCESS;

	ULONG_PTR process_dirbase = CR3[ proc ];
	if ( !process_dirbase )
		return STATUS_UNSUCCESSFUL;

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