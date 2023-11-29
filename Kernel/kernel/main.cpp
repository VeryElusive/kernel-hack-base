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

inline void Delaynie( ULONG milliseconds ) {
    LARGE_INTEGER interval;
    interval.QuadPart = -1 * ( 10000 * milliseconds );  // to 100-nanosecond intervals

    KeDelayExecutionThread( KernelMode, FALSE, &interval );
}

// only for x64
inline void* GetModuleBase( HANDLE gamePID, PROCTYPE proc, wchar_t* moduleName ) {
    SIZE_T read;

    const auto eproc{ Utils::LookupPEProcessFromID( gamePID ) };
    if ( !eproc )
        return 0;

    //DEBUG_PRINT( "basic: %llu\n", Memory::CR3[ proc ] );
    //DEBUG_PRINT( "brute: %llu\n", Memory::CR3[ GAME2 ] );

    PPEB PEB = 0;
    Memory::ReadProcessMemory( proc, reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( eproc ) + 0x550 ), &PEB, 8, &read );

    PPEB_LDR_DATA PEB_Ldr = 0;
    Memory::ReadProcessMemory( proc, GET_ADDRESS_OF_FIELD( PEB, _PEB, Ldr ), &PEB_Ldr, 8, &read );

    LIST_ENTRY ModuleListLoadOrder;
    Memory::ReadProcessMemory( proc, GET_ADDRESS_OF_FIELD( PEB_Ldr, PEB_LDR_DATA, ModuleListLoadOrder ), &ModuleListLoadOrder, sizeof( ModuleListLoadOrder ), &read );

    PLIST_ENTRY list{ ModuleListLoadOrder.Flink };
    PLIST_ENTRY listHead{ ModuleListLoadOrder.Flink };
    do {
        LDR_DATA_TABLE_ENTRY entry;
        Memory::ReadProcessMemory( proc, CONTAINING_RECORD( list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList ), &entry, sizeof( entry ), &read );

        WCHAR modName[ MAX_PATH ] = { 0 };
        Memory::ReadProcessMemory( proc, entry.BaseDllName.Buffer, &modName, entry.BaseDllName.Length * sizeof( WCHAR ), &read );

        if ( entry.BaseDllName.Length ) {
            //Utils::PrintWideString( modName );

            if ( _wcsicmp( modName, moduleName ) == 0 )
                return entry.DllBase;
        }

        list = entry.InLoadOrderModuleList.Flink;
    } while ( listHead != list );

    return NULL;
}


// this one function can exist solely on the stack. i can safely remove entire driver except this function.
NTSTATUS DriverEntry( CommsParse_t* comms ) {
    //DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );

    if ( !comms )
        return STATUS_ABANDONED;

    SIZE_T read = 0;
    HANDLE gamePID{ 0 };

    DataRequest_t req{ };
    char buf[ 64 ]{ };
    void* base{ };

    const auto client{ Utils::LookupPEProcessFromID( comms->m_pClientProcessId ) };
    if ( !client )
        return STATUS_ABANDONED;

    Memory::CR3[ CLIENT ] = *( PULONG_PTR ) ( ( uintptr_t ) client + 0x28 ); //dirbase x64, 32bit is 0x18
    if ( Memory::CR3[ CLIENT ] == 0 ) {
        DWORD UserDirOffset = Memory::GetUserDirectoryTableBaseOffset( );
        Memory::CR3[ CLIENT ] = *( PULONG_PTR ) ( ( uintptr_t ) client + UserDirOffset );
    }

    if ( !Memory::CR3[ CLIENT ] )
        return STATUS_ABANDONED;

    //const auto test{ Memory::BruteForceDirectoryTableBase( comms->m_pClientProcessId ) };


    do {
        if ( Memory::ReadProcessMemory( CLIENT, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        memset( buf, '\0', 64 );

        if ( req.m_iType && req.m_nSize ) {
            if ( req.m_iType == 0xFADED ) {

                return STATUS_SUCCESS;
            }

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( GAME, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( CLIENT, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( CLIENT, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( GAME, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            case REQUEST_GET_PID:
                Memory::ReadProcessMemory( CLIENT, req.m_pAddress, buf, req.m_nSize * sizeof( char ), &read );
                do {
                    gamePID = Utils::GetPIDFromName( reinterpret_cast< char* >( buf ) );
                    Delaynie( 500 );
                } while ( gamePID == 0 );

                //Memory::CR3[ GAME ] = Memory::BruteForceDirectoryTableBase( gamePID );
                Memory::UpdatePML4ECache( gamePID );
                //Memory::UpdateGameCR3( gamePID );

                /*DEBUG_PRINT( "brute: %llu\n", Memory::CR3[ GAME ] );

                Memory::CR3[ GAME ] = *( PULONG_PTR ) ( ( uintptr_t ) Utils::LookupPEProcessFromID( gamePID ) + 0x28 ); //dirbase x64, 32bit is 0x18
                if ( Memory::CR3[ GAME ] == 0 ) {
                    DWORD UserDirOffset = Memory::GetUserDirectoryTableBaseOffset( );
                    Memory::CR3[ GAME ] = *( PULONG_PTR ) ( ( uintptr_t ) Utils::LookupPEProcessFromID( gamePID ) + UserDirOffset );
                }

                DEBUG_PRINT( "basic: %llu\n", Memory::CR3[ GAME ] );
                break;*/
            case REQUEST_GET_MODULE_BASE:
                //Memory::CR3[ GAME ] = Memory::BruteForceDirectoryTableBase( gamePID );
                Memory::ReadProcessMemory( CLIENT, req.m_pAddress, buf, req.m_nSize * sizeof( wchar_t ), &read );

                base = GetModuleBase( gamePID, GAME, reinterpret_cast< wchar_t* >( buf ) );
                Memory::WriteProcessMemory( CLIENT, req.m_pBuffer,
                    &base, 8, &read );

                break;
            default:
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( CLIENT, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read );

            /*Memory::WriteProcessMemory( client,
                GET_ADDRESS_OF_FIELD( comms->m_pBuffer, DataRequest_t, m_iType ), 
                GET_ADDRESS_OF_FIELD( &req, DataRequest_t, m_iType ), sizeof( req.m_iType ), &read );*/

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    } while ( true );

    return STATUS_SUCCESS;
}