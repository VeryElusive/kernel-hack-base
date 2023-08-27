#include <windows.h>
#include <stdio.h>
#include <iostream>

#define IOCTL_NUMBER 0xFADED

struct WriteDataRequest_t {
    DWORD m_pProcessId;
    PVOID m_pAddress;
    PVOID m_pBuffer;
    SIZE_T m_nSize;
};

WriteDataRequest_t writeData;

int da2{ 82 };
int da{ 0 };

// TYPE: 1 BYTE
// ADDRESS: 4 BYTES
// BUFFER: 4 BYTES
// SIZE: 4 BYTES

char communicationBuffer[ 1 + 4 + 4 + 4 ];

int main( void ) {
    HANDLE hDevice;

    hDevice = CreateFile( L"\\\\.\\Havoc", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ) {
        printf( "Failed to open device: %d\n", GetLastError( ) );
        return 1;
    }

    DWORD bytesReturned;
    writeData.m_pBuffer = &da2;
    writeData.m_pAddress = &da;
    writeData.m_nSize = 4;
    writeData.m_pProcessId = GetCurrentProcessId( );

    std::cout << da << std::endl;
    
    if ( !DeviceIoControl( hDevice, IOCTL_NUMBER, &writeData, sizeof( WriteDataRequest_t ), NULL, 0, &bytesReturned, NULL ) ) {
        printf( "Failed to send ioctl command: %d\n", GetLastError( ) );
        CloseHandle( hDevice );
        return 1;
    }

    std::cout << da << std::endl;

    Sleep( 1000 );

    CloseHandle( hDevice );

    return 0;
}