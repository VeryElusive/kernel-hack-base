#pragma once
#include <ntddk.h>
#include <ntimage.h>
#include <intrin.h>
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

    __forceinline BOOLEAN CheckByte( IN UCHAR Byte, IN PCSTR Str )
    {
        if ( Str[ 0 ] == '?' && Str[ 1 ] == '?' )
            return TRUE;

        UCHAR Bytes[ 2 ] = { 0, 0 };
        for ( UCHAR i = 0; i < 2; ++i )
        {
            if ( Str[ i ] >= '0' && Str[ i ] <= '9' )
                Bytes[ i ] = UCHAR( Str[ i ] - 48 );

            else if ( Str[ i ] >= 'A' && Str[ i ] <= 'F' )
                Bytes[ i ] = UCHAR( Str[ i ] - 55 );

            else return FALSE;
        }
        return ( Bytes[ 0 ] << 4 | Bytes[ 1 ] ) == Byte;
    }

    __forceinline BOOLEAN CheckPattern( IN PVOID Base, IN ULONG Size, IN PCSTR Pattern )
    {
        for ( ULONG Offset = 0; Offset < Size; ++Offset, ++Pattern, ++Pattern )
        {
            while ( Pattern[ 0 ] == ' ' )
                ++Pattern;

            if ( Pattern[ 0 ] == 0 )
                return TRUE;

            if ( CheckByte( ( ( PUCHAR ) Base )[ Offset ], Pattern ) == FALSE )
                break;
        }
        return FALSE;
    }

    __forceinline BOOLEAN FindPattern( OUT PVOID* Found, IN PVOID Base, IN ULONG Size, IN PCSTR Pattern )
    {
        for ( ULONG Offset = 0; Offset < Size; ++Offset )
        {
            if ( CheckPattern( ( PUCHAR ) Base + Offset, Size - Offset, Pattern ) == FALSE )
                continue;

            *Found = ( PUCHAR ) Base + Offset;
            return TRUE;
        }
        return FALSE;
    }

    EXTERN_C PLIST_ENTRY PsLoadedModuleList;

#define EX_FIELD_ADDRESS(Type, Base, Member)				((PUCHAR)Base + FIELD_OFFSET(Type, Member))
#define EX_FOR_EACH_IN_LIST(_Type, _Link, _Head, _Current)	for ((_Current) = CONTAINING_RECORD((_Head)->Flink, _Type, _Link); (_Head) != (PLIST_ENTRY)EX_FIELD_ADDRESS(_Type, _Current, _Link); (_Current) = CONTAINING_RECORD(((PLIST_ENTRY)EX_FIELD_ADDRESS(_Type, _Current, _Link))->Flink, _Type, _Link))

    __forceinline PVOID GetSytemModuleBaseAddress( OUT OPTIONAL PVOID* BaseOfImage, IN PCWSTR ImageName ) {
        UNICODE_STRING UnicodeString;
        RtlInitUnicodeString( &UnicodeString, ImageName );

        PKLDR_DATA_TABLE_ENTRY CurrentLdrEntry = NULL;
        EX_FOR_EACH_IN_LIST( KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks, PsLoadedModuleList, CurrentLdrEntry ) {
            if ( ImageName == NULL || RtlEqualUnicodeString( &UnicodeString, &CurrentLdrEntry->BaseDllName, TRUE ) ) {
                if ( BaseOfImage )
                    *BaseOfImage = CurrentLdrEntry->DllBase;

                return CurrentLdrEntry->DllBase;
            }
        }

        return NULL;
    }

    __forceinline BOOLEAN FindPatternImage( OUT PVOID* Found, IN PCWSTR ImageName, IN OPTIONAL PCSTR SectionName, IN PCSTR Pattern )
    {
        PUCHAR ImageBase = NULL;
        if ( GetSytemModuleBaseAddress( ( PVOID* ) &ImageBase, ImageName ) == FALSE )
            return FALSE;

        PIMAGE_DOS_HEADER pIDH = ( PIMAGE_DOS_HEADER ) ImageBase;
        if ( pIDH->e_magic != IMAGE_DOS_SIGNATURE )
            return FALSE;

        PIMAGE_NT_HEADERS pINH = ( PIMAGE_NT_HEADERS ) &ImageBase[ pIDH->e_lfanew ];
        if ( pINH->Signature != IMAGE_NT_SIGNATURE )
            return FALSE;

        PIMAGE_SECTION_HEADER pISH = PIMAGE_SECTION_HEADER( pINH + 1 );
        for ( USHORT i = 0; i < pINH->FileHeader.NumberOfSections; ++i )
        {
            if ( pISH[ i ].Characteristics & IMAGE_SCN_MEM_DISCARDABLE )
                continue;

            if ( ( pISH[ i ].Characteristics & IMAGE_SCN_MEM_EXECUTE ) == NULL )
                continue;

            if ( SectionName && strcmp( SectionName, ( const char* ) pISH[ i ].Name ) != 0 )
                continue;

            if ( FindPattern( Found, &ImageBase[ pISH[ i ].VirtualAddress ], pISH[ i ].Misc.VirtualSize, Pattern ) )
                return TRUE;
        }
        return FALSE;
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

    __forceinline bool IsSubstring( const char* mainString, const char* substring ) {
        size_t mainLen = strlen( mainString );
        size_t subLen = strlen( substring );

        // If substring is longer than the main string, it cannot be a substring
        if ( subLen > mainLen ) {
            return false;
        }

        for ( int i = 0; i <= mainLen - subLen; ++i ) {
            // Check if the substring matches the current portion of the main string
            if ( strncmp( mainString + i, substring, subLen ) == 0 ) {
                return true;  // Substring found
            }
        }

        return false;  // Substring not found
    }

    __forceinline void ToLowerCase( char* str ) {
        while ( *str ) {
            *str = char( tolower( ( unsigned char ) *str ) );
            str++;
        }
    }

#define ImageFileName 0x5A8 // EPROCESS::ImageFileName
#define ActiveThreads 0x5F0 // EPROCESS::ActiveThreads
#define ThreadListHead 0x5E0 // EPROCESS::ThreadListHead
#define ActiveProcessLinks 0x448 // EPROCESS::ActiveProcessLinks
#define SectionBaseAddress 0x520 // EPROCESS::SectionBaseAddress

#define GetPEProcessMember( peprocess, offset, output ) RtlCopyMemory( ( PVOID ) ( &output ), ( PVOID ) ( ( uintptr_t ) peprocess + offset ), sizeof( output ) );

    __forceinline PEPROCESS GetProcessFromName( CHAR* processName, PEPROCESS exclude = 0 ) {
        CHAR image_name[ 15 ];
        PEPROCESS sys_process = PsInitialSystemProcess;
        PEPROCESS cur_entry = sys_process;
        DWORD active_threads;

        do {
            GetPEProcessMember( cur_entry, ImageFileName, image_name );

            //PrintString( image_name );

            if ( exclude != cur_entry ) {
                if ( strcmp( image_name, processName ) == 0 ) {
                    GetPEProcessMember( cur_entry, ActiveThreads, active_threads );

                    if ( active_threads )
                        return cur_entry;
                }
            }

            PLIST_ENTRY list = ( PLIST_ENTRY ) ( ( uintptr_t ) ( cur_entry ) + ActiveProcessLinks );
            cur_entry = ( PEPROCESS ) ( ( uintptr_t ) list->Flink - ActiveProcessLinks );

        } while ( cur_entry != sys_process );

        return 0;
    }

    __forceinline HANDLE GetPIDFromName( CHAR* processName, PEPROCESS exclude = 0 ) {
        const auto proc{ GetProcessFromName( processName, exclude ) };
        if ( !proc )
            return 0;

        return PsGetProcessId( proc );
    }

    __forceinline PEPROCESS LookupPEProcessFromID( HANDLE pid ) {
        PEPROCESS sys_process = PsInitialSystemProcess;
        PEPROCESS cur_entry = sys_process;
        DWORD active_threads;

        do {
            GetPEProcessMember( cur_entry, ActiveThreads, active_threads );

            if ( active_threads ) {
                if ( pid == PsGetProcessId( cur_entry ) )
                    return cur_entry;
            }

            PLIST_ENTRY list = ( PLIST_ENTRY ) ( ( uintptr_t ) ( cur_entry ) +ActiveProcessLinks );
            cur_entry = ( PEPROCESS ) ( ( uintptr_t ) list->Flink - ActiveProcessLinks );

        } while ( cur_entry != sys_process );

        return 0;
    }

    __forceinline void* GetNtosImageBase( ULONG* Size ) {
        UCHAR EntryPoint[ ] = { 0x48, 0x8D, 0x1D, 0xFF };
        for ( auto Page = __readmsr( 0xC0000082 ) & ~0xfff; Page != NULL; Page -= PAGE_SIZE )
        {
            if ( *( USHORT* ) Page == IMAGE_DOS_SIGNATURE )
            {
                for ( auto Bytes = Page; Bytes < Page + 0x400; Bytes += 8 )
                {
                    if ( memcmp( ( void* ) Bytes, EntryPoint, sizeof( EntryPoint ) ) )
                    {
                        *Size = reinterpret_cast< IMAGE_NT_HEADERS64* >( Page + reinterpret_cast< IMAGE_DOS_HEADER* >( Page )->e_lfanew )->OptionalHeader.SizeOfImage;
                        return ( void* ) Page;
                    }
                }
            }
        }

        return nullptr;
    }
}