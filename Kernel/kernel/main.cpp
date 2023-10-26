#pragma warning ( disable : 4100 )

#include "utils/memory.h"
#include "utils/utils.h"

#include "../../shared_structs.h"
#include <ntddk.h>

// TODO: when u can be fucked, go through and only map this one function and just parse in the imports in the driverentry in a buffer

#define DEBUG_PRINT( msg, ... ) DbgPrintEx( 0, 0, msg, __VA_ARGS__ );

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

    PEPROCESS clientProcess = NULL;
    PEPROCESS gameProcess = NULL;
    if ( !comms->m_pGameProcessId || !comms->m_pClientProcessId )
        return 1;

    NTSTATUS status{ PsLookupProcessByProcessId( ( HANDLE ) comms->m_pGameProcessId, &gameProcess ) };
    if ( status != STATUS_SUCCESS )
        return 1;

    status = PsLookupProcessByProcessId( ( HANDLE ) comms->m_pGameProcessId, &clientProcess );
    if ( status != STATUS_SUCCESS )
        return 1;

    do {
        read = 0;
        DataRequest_t req{ };

        // TODO: make this only init physical addresses once or woteva
        if ( Memory::ReadProcessMemory( clientProcess, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            if ( req.m_nSize == 0xFADED ) {
                //DEBUG_PRINT( "[ HAVOC ] exiting.\n" );
                break;
            }

            char buf[ 16 ]{ };

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( gameProcess, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( clientProcess, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( clientProcess, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( gameProcess, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( clientProcess, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read );

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    } while ( true );

    return STATUS_SUCCESS;
}