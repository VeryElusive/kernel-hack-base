#pragma warning ( disable : 4100 )

#include "utils/memory.h"
#include "utils/utils.h"

#include "../../shared_structs.h"
#include <ntddk.h>

/*
* BSOD if you don't call PsLookupProcessByProcessId
* so when we start modifying mem of a different process, see if we can just call PsLookupProcessByProcessId on client pid.
*/

// TODO: when u can be fucked, go through and only map this one function and just parse in the imports in the driverentry in a buffer

#define DEBUG_PRINT( msg, ... ) DbgPrintEx( 0, 0, msg, __VA_ARGS__ );

#define GET_ADDRESS_OF_FIELD(address, type, field) reinterpret_cast< void* >((type *)( \
                                                  (PCHAR)(address) + \
                                                  (ULONG_PTR)(&((type *)0)->field)))

#define INVALID_HANDLE ((HANDLE)(LONG_PTR)-1)

void PrintWideString( const wchar_t* wideString ) {
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

// only for x64
inline void* GetModuleBase( CommsParse_t* comms, HANDLE gamePID, wchar_t* moduleName ) {
    SIZE_T read;

    PEPROCESS proc{ };
    if ( !comms )
        return nullptr;

    if ( PsLookupProcessByProcessId( gamePID, &proc ) != STATUS_SUCCESS || !proc )
        return nullptr;

    PPEB PEB = 0;
    Memory::ReadProcessMemory( gamePID, reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( proc ) + 0x550 ), &PEB, 8, &read );
    
    PPEB_LDR_DATA PEB_Ldr = 0;
    Memory::ReadProcessMemory( gamePID, GET_ADDRESS_OF_FIELD( PEB, _PEB, Ldr ), &PEB_Ldr, 8, &read );

    LIST_ENTRY ModuleListLoadOrder;
    Memory::ReadProcessMemory( gamePID, GET_ADDRESS_OF_FIELD( PEB_Ldr, PEB_LDR_DATA, ModuleListLoadOrder ), &ModuleListLoadOrder, sizeof( ModuleListLoadOrder ), &read );

    //DEBUG_PRINT( "searching for module %s\n", moduleName );

    PLIST_ENTRY list{ ModuleListLoadOrder.Flink };
    PLIST_ENTRY listHead{ ModuleListLoadOrder.Flink };
    do {
        LDR_DATA_TABLE_ENTRY entry;
        Memory::ReadProcessMemory( gamePID, CONTAINING_RECORD( list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList ), &entry, sizeof( entry ), &read );

        if ( entry.BaseDllName.Length ) {
            WCHAR modName[ MAX_PATH ] = { 0 };
            Memory::ReadProcessMemory( gamePID, entry.BaseDllName.Buffer, &modName, entry.BaseDllName.Length * sizeof( WCHAR ), &read );
            PrintWideString( modName );

            if ( _wcsicmp( modName, moduleName ) == 0 )
                return entry.DllBase;
        }

        list = entry.InLoadOrderModuleList.Flink;
    } while ( listHead != list );

    return NULL;
}

#define ImageFileName 0x5A8 // EPROCESS::ImageFileName
#define ActiveThreads 0x5F0 // EPROCESS::ActiveThreads
#define ThreadListHead 0x5E0 // EPROCESS::ThreadListHead
#define ActiveProcessLinks 0x448 // EPROCESS::ActiveProcessLinks

template <typename str_type, typename str_type_2>
__forceinline bool crt_strcmp( str_type str, str_type_2 in_str, bool two )
{
    if ( !str || !in_str )
        return false;

    wchar_t c1, c2;
#define to_lower(c_char) ((c_char >= 'A' && c_char <= 'Z') ? (c_char + 32) : c_char)

    do
    {
        c1 = *str++; c2 = *in_str++;
        c1 = to_lower( c1 ); c2 = to_lower( c2 );

        if ( !c1 && ( two ? !c2 : 1 ) )
            return true;

    } while ( c1 == c2 );

    return false;
}

inline HANDLE get_process_id_by_name( const wchar_t* process_name )
{
    CHAR image_name[ 15 ];
    PEPROCESS sys_process = PsInitialSystemProcess;
    PEPROCESS cur_entry = sys_process;

    do
    {
        RtlCopyMemory( ( PVOID ) ( &image_name ), ( PVOID ) ( ( uintptr_t ) cur_entry + ImageFileName ), sizeof( image_name ) );

        if ( crt_strcmp( image_name, process_name, true ) )
        {
            DWORD active_threads;
            RtlCopyMemory( ( PVOID ) &active_threads, ( PVOID ) ( ( uintptr_t ) cur_entry + ActiveThreads ), sizeof( active_threads ) );

            if ( active_threads )
                return PsGetProcessId( cur_entry );
        }

        PLIST_ENTRY list = ( PLIST_ENTRY ) ( ( uintptr_t ) ( cur_entry ) +ActiveProcessLinks );
        cur_entry = ( PEPROCESS ) ( ( uintptr_t ) list->Flink - ActiveProcessLinks );

    } while ( cur_entry != sys_process );

    return INVALID_HANDLE;
}

// this one function can exist solely on the stack. i can safely remove entire driver except this function.
NTSTATUS DriverEntry( CommsParse_t* comms ) {
    //DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );

    if ( !comms ) {
        //DEBUG_PRINT( "[ HAVOC ] comms was null!\n" );
        return STATUS_ABANDONED;
    }

    SIZE_T read = 0;
    HANDLE gamePID{ INVALID_HANDLE };

    do {
        DataRequest_t req{ };
        char buf[ 64 ]{ };
        void* base{ };

        // TODO: make this only init physical addresses once or woteva
        if ( Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_nSize ) {
            if ( req.m_iType == 0xFADED ) {
                //DEBUG_PRINT( "[ HAVOC ] exiting.\n" );
                break;
            }

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( ( HANDLE ) gamePID, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) gamePID, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            case REQUEST_GET_PID:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pAddress, buf, req.m_nSize * sizeof( wchar_t ), &read );
                do {
                    gamePID = get_process_id_by_name( reinterpret_cast< wchar_t* >( buf ) );
                } while ( gamePID == INVALID_HANDLE );
                break;
            case REQUEST_GET_MODULE_BASE:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pAddress, buf, req.m_nSize * sizeof( wchar_t ), &read );

                base = GetModuleBase( comms, gamePID, reinterpret_cast< wchar_t* >( buf ) );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer,
                    &base, 8, &read );
                break;
            default:
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read );

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    } while ( true );

    return STATUS_SUCCESS;
}