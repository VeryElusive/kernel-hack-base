#pragma warning ( disable : 4100 )

#include "utils/memory.h"
#include "utils/hook.h"
#include "utils/utils.h"
#include "communication/communication.h"

#include "../../shared_structs.h"

#define DEBUG_PRINT( msg, ... ) DbgPrintEx( 0, 0, msg, __VA_ARGS__ );

// this one function can exist solely on the stack. i can safely remove entire driver EXCEPT for this function.
void __stdcall WorkerThread( void* base, CommsParse_t* comms ) {
    if ( !comms ) {
        DEBUG_PRINT( "[ HAVOC ] comms was null!\n" );
        return;
    }

    // now we NOP all driver memory except for this function, etc

    do {
        DataRequest_t req{ };
        SIZE_T read;
        if ( Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            char buf[ 16 ]{ };

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pGameProcessId, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( ( HANDLE ) comms->m_pClientProcessId, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) comms->m_pGameProcessId, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( ( HANDLE ) comms->m_pClientProcessId, comms->m_pBuffer, &req, sizeof( DataRequest_t ), &read );

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    } while ( true );
}

NTSTATUS DriverEntry( void* base, CommsParse_t* comms ) {
    DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );

    WorkerThread( base, comms );

    return STATUS_SUCCESS;
}