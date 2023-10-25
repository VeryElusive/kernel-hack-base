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
	__forceinline PVOID GetModuleBaseAddress( PCHAR name ) {
		PVOID addr = 0;

		ULONG size = 0;
		NTSTATUS status = ZwQuerySystemInformation( SystemModuleInformation, 0, 0, &size );
		if ( STATUS_INFO_LENGTH_MISMATCH != status ) {
			return addr;
		}

		auto modules{ reinterpret_cast< PSYSTEM_MODULE_INFORMATION >( ExAllocatePool2( POOL_FLAG_NON_PAGED, size, 'HVC' ) ) };
		if ( !modules ) {
			return addr;
		}

		if ( !NT_SUCCESS( status = ZwQuerySystemInformation( SystemModuleInformation, modules, size, 0 ) ) ) {
			ExFreePool( modules );
			return addr;
		}

		for ( ULONG i = 0; i < modules->NumberOfModules; ++i ) {
			SYSTEM_MODULE m = modules->Modules[ i ];

			if ( strstr( ( PCHAR ) m.FullPathName, name ) ) {
				addr = m.ImageBase;
				break;
			}
		}

		ExFreePool( modules );
		return addr;
	}


	__forceinline BOOLEAN CheckMask( PCHAR base, PCHAR pattern, PCHAR mask ) {
		for ( ; *mask; ++base, ++pattern, ++mask ) {
			if ( *mask == 'x' && *base != *pattern ) {
				return FALSE;
			}
		}

		return TRUE;
	}


	__forceinline PVOID FindPattern( PCHAR base, DWORD length, PCHAR pattern, PCHAR mask ) {
		length -= ( DWORD ) strlen( mask );
		for ( DWORD i = 0; i <= length; ++i ) {
			PCHAR addr = &base[ i ];
			if ( CheckMask( addr, pattern, mask ) ) {
				return addr;
			}
		}

		return 0;
	}

	__forceinline PVOID FindPatternImage( PCHAR base, PCHAR pattern, PCHAR mask ) {
		PVOID match = 0;

		PIMAGE_NT_HEADERS headers = ( PIMAGE_NT_HEADERS ) ( base + ( ( PIMAGE_DOS_HEADER ) base )->e_lfanew );
		PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION( headers );
		for ( DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i ) {
			PIMAGE_SECTION_HEADER section = &sections[ i ];
			if ( *( PINT ) section->Name == 'EGAP' || memcmp( section->Name, ".text", 5 ) == 0 ) {
				match = FindPattern( base + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask );
				if ( match ) {
					break;
				}
			}
		}

		return match;
	}
}