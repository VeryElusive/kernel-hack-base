#include <windows.h>
#include <stdio.h>
#include <iostream>

#define IOCTL_NUMBER 0xFADED

int da2{ 82 };
int da{ 0 };

struct DataRequest_t {
    char m_iType{ };
    void* m_pAddress{ };
    void* m_pBuffer{ };
    int m_nSize{ };
};

DataRequest_t communicationBuffer;

struct CommsParse_t {
    DWORD m_pProcessId;
    DataRequest_t* m_pCommsBuffer;
};
CommsParse_t comms{ };

void Write( void* address, void* buffer, int size ) {
    communicationBuffer.m_iType = 1;
    communicationBuffer.m_pAddress = address;
    communicationBuffer.m_pBuffer = buffer;
    communicationBuffer.m_nSize = size;
}

int main( void ) {
    HANDLE hDevice;

    hDevice = CreateFile( L"\\\\.\\Havoc", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hDevice == INVALID_HANDLE_VALUE ) {
        printf( "Failed to open device: %d\n", GetLastError( ) );
        return 1;
    }

    std::cout << da << std::endl;

    DWORD bytesReturned;
    comms.m_pProcessId = GetCurrentProcessId( );
    comms.m_pCommsBuffer = &communicationBuffer;

    if ( !DeviceIoControl( hDevice, IOCTL_NUMBER, &comms, sizeof( CommsParse_t ), NULL, 0, &bytesReturned, NULL ) ) {
        printf( "Failed to send comms: %d\n", GetLastError( ) );
        CloseHandle( hDevice );
        return 1;
    }

    Write( &da, &da2, 4 );

    while ( true ) {
        std::cout << da << std::endl;

        Sleep( 1000 );
    }

    CloseHandle( hDevice );

    return 0;
}