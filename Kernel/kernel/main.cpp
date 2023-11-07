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

UNICODE_STRING ConvertToUnicodeString( const wchar_t* wcharArray ) {
    UNICODE_STRING unicodeString;
    RtlInitUnicodeString( &unicodeString, wcharArray );
    return unicodeString;
}

void PrintUnicodeString( UNICODE_STRING unicodeString ) {
    if ( unicodeString.Buffer ) {
        char narrowString[ MAX_PATH ];
        int i;
        for ( i = 0; i < 64; i++ ) {
            narrowString[ i ] = static_cast< char >( unicodeString.Buffer[ i ] & 0xFF );
        }
        narrowString[ i ] = '\0';
        DEBUG_PRINT( "UNICODE_STRING: %s\n", narrowString );
    }
}


// only for x64
inline void* GetModuleBase( CommsParse_t* comms, UNICODE_STRING moduleName ) {
    PEPROCESS proc{ };
    if ( !comms ) {
        DEBUG_PRINT( "comms fail.\n" );
        return nullptr;
    }

    if ( PsLookupProcessByProcessId( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), &proc ) != STATUS_SUCCESS || !proc ) {
        DEBUG_PRINT( "PsLookupProcessByProcessId fail.\n" );
        return nullptr;
    }

    SIZE_T read;

    PPEB PEB = 0;
    Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( proc ) + 0x550 ), &PEB, 8, &read );
    
    PPEB_LDR_DATA PEB_Ldr = 0;
    Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), GET_ADDRESS_OF_FIELD( PEB, _PEB, Ldr ), &PEB_Ldr, 8, &read );

    LIST_ENTRY ModuleListLoadOrder;
    Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), GET_ADDRESS_OF_FIELD( PEB_Ldr, PEB_LDR_DATA, ModuleListLoadOrder ), &ModuleListLoadOrder, sizeof( ModuleListLoadOrder ), &read );

    DEBUG_PRINT( "searching for module %wZ\n", moduleName );

    int count = 0;
    PLIST_ENTRY list{ ModuleListLoadOrder.Flink };
    while ( list != reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( PEB_Ldr ) + 0xD ) && count < 99 ) {
        // CONTAINING_RECORD expansion:
        //const auto test = ( ( LDR_DATA_TABLE_ENTRY* ) ( ( PCHAR ) ( list ) -( ULONG_PTR ) ( &( ( LDR_DATA_TABLE_ENTRY* ) 0 )->InLoadOrderModuleList )));
        PLDR_DATA_TABLE_ENTRY entry;
        Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), CONTAINING_RECORD( list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList ), &entry, sizeof( entry ), &read );

        UNICODE_STRING mod;
        Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), GET_ADDRESS_OF_FIELD( entry, LDR_DATA_TABLE_ENTRY, BaseDllName ), &mod, sizeof( mod ), &read );

        WCHAR pbModName[ 64 ] = { 0 };
        Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), mod.Buffer, &pbModName, sizeof( pbModName ), &read );

        mod.Buffer = reinterpret_cast< PWCH >( pbModName );
        PrintUnicodeString( mod );

        if ( RtlCompareUnicodeString( &mod, &moduleName, TRUE ) == NULL ) {
            DEBUG_PRINT( "found.\n" );
            break;
        }

        //list->Flink
        void* next = nullptr;
        Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), GET_ADDRESS_OF_FIELD( list, LIST_ENTRY, Flink ), &next, 8, &read );

        Memory::ReadProcessMemory( reinterpret_cast< HANDLE >( comms->m_pGameProcessId ), next, &list, 8, &read );
        count++;
        //break;
    }

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

        if ( req.m_iType && req.m_nSize ) {
            if ( req.m_nSize == 0xFADED ) {
                //DEBUG_PRINT( "[ HAVOC ] exiting.\n" );
                break;
            }

            char buf[ 32 ]{ };
            void* base{ };

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

                base = GetModuleBase( comms, ConvertToUnicodeString( reinterpret_cast< const wchar_t* >( buf ) ) );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer,
                    &base,
                    8, &read );
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