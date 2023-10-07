#pragma once
#include <ntddk.h>
#include <ntimage.h>
#include "../sdk/windows/ntstructs.h"

namespace Utils {
	__forceinline CHAR* LowerStr( CHAR* Str ) {
		for ( CHAR* S = Str; *S; ++S ) {
			*S = ( CHAR ) tolower( *S );
		}
		return Str;

	}
	template <typename T = PVOID>
	__forceinline T GetModuleInfo( const char* Name, DWORD* OutSize = nullptr ) {
		PVOID Base{ nullptr };
		DWORD RequiredSize{ 0 };

		if ( ZwQuerySystemInformation( SystemModuleInformation,
			nullptr,
			NULL,
			&RequiredSize ) != STATUS_INFO_LENGTH_MISMATCH ) {

			return reinterpret_cast< T >( nullptr );
		}

		auto Modules{ reinterpret_cast< SYSTEM_MODULE_INFORMATION* >( ExAllocatePool2( NonPagedPool, RequiredSize, 'HVC' ) ) };

		if ( !Modules ) {
			return reinterpret_cast< T >( nullptr );
		}

		if ( !NT_SUCCESS( ZwQuerySystemInformation( SystemModuleInformation,
			Modules,
			RequiredSize,
			nullptr ) ) ) {
			ExFreePool( Modules );
			return reinterpret_cast< T >( nullptr );
		}

		for ( DWORD i = 0; i < Modules->NumberOfModules; ++i ) {
			SYSTEM_MODULE CurModule{ Modules->Modules[ i ] };

			if ( strstr( Utils::LowerStr( ( CHAR* ) CurModule.FullPathName ), Name ) )
			{
				Base = CurModule.ImageBase;

				if ( OutSize ) {
					*OutSize = CurModule.ImageSize;
				}

				break;
			}
		}

		ExFreePool( Modules );
		return reinterpret_cast< T >( Base );
	}

	__forceinline BOOLEAN CheckMask( const char* Base, const char* Pattern, const char* Mask ) {
		for ( ; *Mask; ++Base, ++Pattern, ++Mask ) {
			if ( *Mask == 'x' && *Base != *Pattern ) {
				return FALSE;
			}
		}

		return TRUE;
	}

	__forceinline const char* FindPattern( const char* Base, DWORD Length, const char* Pattern, const char* Mask ) {
		Length -= ( DWORD ) strlen( Mask );

		for ( DWORD i = 0; i <= Length; ++i ) {
			auto Addr{ &Base[ i ] };

			if ( CheckMask( Addr, Pattern, Mask ) ) {
				return Addr;
			}
		}

		return 0;
	}

	__forceinline const char* FindPatternImage( const char* Base, const char* Pattern, const char* Mask ) {
		const char* Match{ 0 };

		IMAGE_NT_HEADERS* Headers{ ( PIMAGE_NT_HEADERS ) ( Base + ( ( PIMAGE_DOS_HEADER ) Base )->e_lfanew ) };
		IMAGE_SECTION_HEADER* Sections{ IMAGE_FIRST_SECTION( Headers ) };

		for ( DWORD i = 0; i < Headers->FileHeader.NumberOfSections; ++i ) {
			IMAGE_SECTION_HEADER* Section{ &Sections[ i ] };

			if ( *( INT* ) Section->Name == 'EGAP' || memcmp( Section->Name, ".text", 5 ) == 0 ) {
				Match = FindPattern( Base + Section->VirtualAddress, Section->Misc.VirtualSize, Pattern, Mask );

				if ( Match ) {
					break;
				}
			}
		}

		return Match;
	}

	__forceinline bool WriteToReadOnlyMemory( void* address, void* buffer, size_t size ) {
		PMDL Mdl = IoAllocateMdl( address, (ULONG)size, FALSE, FALSE, NULL );
		if ( !Mdl )
			return false;

		MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
		PVOID Mapping = MmMapLockedPagesSpecifyCache( Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority );
		MmProtectMdlSystemAddress( Mdl, PAGE_READWRITE );

		memcpy( Mapping, buffer, size );
		MmUnmapLockedPages( Mapping, Mdl );
		MmUnlockPages( Mdl );
		IoFreeMdl( Mdl );

		return true;
	}
}