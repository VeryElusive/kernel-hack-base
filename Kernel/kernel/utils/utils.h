#pragma once
#include <ntddk.h>
#include <ntimage.h>
#include "../sdk/windows/ntstructs.h"

#define GET_ADDRESS_OF_FIELD(address, type, field) reinterpret_cast< void* >((type *)( \
                                                  (PCHAR)(address) + \
                                                  (ULONG_PTR)(&((type *)0)->field)))

namespace Utils {
	__forceinline CHAR* LowerStr( CHAR* Str ) {
		for ( CHAR* S = Str; *S; ++S ) {
			*S = ( CHAR ) tolower( *S );
		}
		return Str;

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

    __forceinline void PrintWideString( const wchar_t* wideString ) {
        if ( wideString ) {
            char narrowString[ 64 ];
            int i;
            for ( i = 0; i < 63; i++ ) {
                narrowString[ i ] = static_cast< char >( wideString[ i ] & 0xFF );

                if ( narrowString[ i ] == '\0' )
                    break;
            }
            narrowString[ i ] = '\0';
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Wide string: %s\n", narrowString );
        }
    }

    __forceinline void PrintString( const CHAR* str ) {
        if ( str ) {
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Str: %s\n", str );
        }
    }

#define ImageFileName 0x5A8 // EPROCESS::ImageFileName
#define ActiveThreads 0x5F0 // EPROCESS::ActiveThreads
#define ThreadListHead 0x5E0 // EPROCESS::ThreadListHead
#define ActiveProcessLinks 0x448 // EPROCESS::ActiveProcessLinks

	HANDLE GetPIDFromName( const CHAR* processName ) {
        CHAR image_name[ 15 ];
        PEPROCESS sys_process = PsInitialSystemProcess;
        PEPROCESS cur_entry = sys_process;

        do {
            RtlCopyMemory( ( PVOID ) ( &image_name ), ( PVOID ) ( ( uintptr_t ) cur_entry + ImageFileName ), sizeof( image_name ) );

            PrintString( image_name );

            if ( strcmp( image_name, processName ) ) {
                DWORD active_threads;
                RtlCopyMemory( ( PVOID ) &active_threads, ( PVOID ) ( ( uintptr_t ) cur_entry + ActiveThreads ), sizeof( active_threads ) );

				if ( active_threads )
					return reinterpret_cast< HANDLE >( reinterpret_cast< uintptr_t >( cur_entry ) + 0x440 );//PsGetProcessId( cur_entry );
            }

            PLIST_ENTRY list = ( PLIST_ENTRY ) ( ( uintptr_t ) ( cur_entry ) +ActiveProcessLinks );
            cur_entry = ( PEPROCESS ) ( ( uintptr_t ) list->Flink - ActiveProcessLinks );

        } while ( cur_entry != sys_process );

        return 0;
    }
}