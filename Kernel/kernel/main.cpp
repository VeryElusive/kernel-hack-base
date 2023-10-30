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

UNICODE_STRING ConvertToUnicodeString( const wchar_t* wcharArray, int size ) {
    UNICODE_STRING unicodeString;
    RtlInitUnicodeString( &unicodeString, wcharArray );
    return unicodeString;
}

// only for x64
inline void* GetModuleBase( CommsParse_t* comms, UNICODE_STRING moduleMame ) {
    PEPROCESS proc{ };
    if ( !comms )
        return nullptr;

    if ( PsLookupProcessByProcessId( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), &proc ) != STATUS_SUCCESS || !proc )
        return nullptr;

    const PPEB pPeb{ PsGetProcessPeb( proc ) };
    if ( !pPeb )
        return NULL;

    KAPC_STATE state;

    KeStackAttachProcess( proc, &state );

    PPEB_LDR_DATA pLdr{ ( PPEB_LDR_DATA ) pPeb->Ldr };

    if ( !pLdr ) {
        KeUnstackDetachProcess( &state );
        return NULL;
    }

    for ( PLIST_ENTRY list = ( PLIST_ENTRY ) pLdr->ModuleListLoadOrder.Flink; list != &pLdr->ModuleListLoadOrder; list = ( PLIST_ENTRY ) list->Flink ) {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD( list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList );

        if ( RtlCompareUnicodeString( &pEntry->BaseDllName, &moduleMame, TRUE ) == NULL ) {
            void* baseAddr = pEntry->DllBase;
            KeUnstackDetachProcess( &state );
            return baseAddr;
        }
    }

    KeUnstackDetachProcess( &state );
    return NULL;
}

// this one function can exist solely on the stack. i can safely remove entire driver except this function.
NTSTATUS DriverEntry( CommsParse_t* comms ) {
    //DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );

    if ( !comms ) {
        //DEBUG_PRINT( "[ HAVOC ] comms was null!\n" );
        return 1;
    }

    SIZE_T read = 0;
    //const auto base{ ( void* ) ( ( SIZE_T ) DriverEntry - comms->m_iEntryDeltaFromBase ) };
    //memset( base, 0xCC, comms->m_iEntryDeltaFromBase );

    do {
        read = 0;
        DataRequest_t req{ };

        // TODO: make this only init physical addresses once or woteva
        if ( Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            if ( req.m_nSize == 0xFADED ) {
                //DEBUG_PRINT( "[ HAVOC ] exiting.\n" );
                break;
            }

            char buf[ 32 ]{ };

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pGameProcessId, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pGameProcessId, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            case REQUEST_GET_MODULE_BASE:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pAddress, buf, req.m_nSize * sizeof( wchar_t ), &read );

                void* base{ GetModuleBase( comms, ConvertToUnicodeString( reinterpret_cast< const wchar_t* >( buf ), req.m_nSize ) ) };
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, base, 8, &read );
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read );

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    } while ( true );

    return STATUS_SUCCESS;
}