#pragma warning ( disable : 4100 )

#include "utils/memory.h"
#include "utils/hook.h"
#include "utils/utils.h"
#include "communication/communication.h"

#include "../../shared_structs.h"

INT64( NTAPI* EnumerateDebuggingDevicesOriginal )( PVOID, PVOID );
DWORD64 gFunc{};

#define DEBUG_PRINT( msg, ... ) DbgPrintEx( 0, 0, msg, __VA_ARGS__ );

// this one function can exist solely on the stack. i can safely remove entire driver EXCEPT for this function.
VOID WorkerThread( char* CommunicationBuffer, int GamePID, int ClientPID ) {
    if ( !CommunicationBuffer || !GamePID || !ClientPID )
        return;

    // this is inside hook, now we unhook, unload driver, etc

    while ( true ) {
        DataRequest_t req{ };
        SIZE_T read;
        if ( Memory::ReadProcessMemory( ( HANDLE ) ClientPID, CommunicationBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            char buf[ 16 ]{ };

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( ( HANDLE ) GamePID, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) ClientPID, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( ( HANDLE ) ClientPID, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( ( HANDLE ) GamePID, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            }

            req.m_iType = 0;
            Memory::WriteProcessMemory( ( HANDLE ) ClientPID, CommunicationBuffer, &req, sizeof( DataRequest_t ), &read );

            //DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
        }
    }
}

#define RVA(addr, size) (const char*)addr + *(INT*)((BYTE*)addr + ((size) - 4)) + size

INT64 NTAPI EnumerateDebuggingDevicesHook( CommsParse_t* a1, PINT64 a2 ) {
    DEBUG_PRINT( "[ HAVOC ] detour called!\n" );
    if ( ExGetPreviousMode( ) != UserMode
        || a1 == nullptr
        || a1->m_iSignage != 0xFADED ) {
        return EnumerateDebuggingDevicesOriginal( a1, a2 );
    }

    DEBUG_PRINT( "[ HAVOC ] Communication established with usermode!\n" );

    // NtConvertBetweenAuxiliaryCounterAndPerformanceCounter() was called by the usermode client

    // unhook
    InterlockedExchangePointer( ( PVOID* ) gFunc, ( PVOID ) EnumerateDebuggingDevicesOriginal );

    WorkerThread( reinterpret_cast< char* >( a1->m_pBuffer ), a1->m_pGameProcessId, a1->m_pClientProcessId );

    return 1;
}

NTSTATUS DriverEntry( ) {
    DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );
    
    // place hooks

    if ( const auto gKernelBase = Utils::GetModuleInfo<char*>( "ntoskrnl.exe" ) ) {
        if ( auto Func = Utils::FindPatternImage( gKernelBase,
            "\x48\x8B\x05\x00\x00\x00\x00\x75\x07\x48\x8B\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00",
            "xxx????xxxxx????x????" ) ) {

            gFunc = ( DWORD64 ) ( Func = RVA( Func, 7 ) );
            *( PVOID* ) &EnumerateDebuggingDevicesOriginal = InterlockedExchangePointer( ( PVOID* ) Func, ( PVOID ) EnumerateDebuggingDevicesHook ); // Hook EnumerateDebuggingDevices()

            DEBUG_PRINT( "[ HAVOC ] hooked EnumerateDebuggingDevices!\n" );
            return STATUS_SUCCESS;
        }
        else
            DEBUG_PRINT( "[ HAVOC ] failed to find EnumerateDebuggingDevices!\n" );
    }
    else {
        DEBUG_PRINT( "[ HAVOC ] failed to find gKernelBase!\n" );
    }


    // now call the hooked function from usermode

    return STATUS_SUCCESS;
}