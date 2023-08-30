#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "utils/memory.h"

int da2{ 420 };
int da{ 0 };

int main( void ) {
    HANDLE hDevice;

    hDevice = CreateFile( L"\\\\.\\Havoc", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ) {
        printf( "Failed to open device: %d\n", GetLastError( ) );
        return 1;
    }

    std::cout << "PRE: " << da << std::endl;

    DWORD bytesReturned;
    Context::Comms.m_pProcessId = GetCurrentProcessId( );
    //comms.m_pGameProcessId = comms.m_pControlProcessId; // TODO: WHY DOES THIS FAIL?!??!
    Context::Comms.m_pBuffer = &Context::CommunicationBuffer;

    if ( !DeviceIoControl( hDevice, IOCTL_NUMBER, &Context::Comms, sizeof( CommsParse_t ), NULL, 0, &bytesReturned, NULL ) ) {
        printf( "Failed to send comms: %d\n", GetLastError( ) );
        CloseHandle( hDevice );
        return 1;
    }

    Memory::Write( &da, &da2, 4 );

    std::cout << "POST: " << da << std::endl;

    Sleep( 10000 );

    CloseHandle( hDevice );

    return 0;
}