#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "utils/memory.h"
#include "mapper/mapper.h"
#include "ext/intel_driver.h"

int da2{ 420 };
int da{ 0 };

bool Initialise( ) {
	Context::Comms.m_pGameProcessId = GetCurrentProcessId( );
	Context::Comms.m_pClientProcessId = GetCurrentProcessId( );
	Context::Comms.m_iSignage = 0xFADED;
	Context::Comms.m_pBuffer = &Context::CommunicationBuffer;

	HANDLE iqvw64e_device_handle = intel_driver::Load( );

	if ( iqvw64e_device_handle == INVALID_HANDLE_VALUE )
		return false;

	std::vector<uint8_t> raw_image = { 0 };
	const std::wstring driver_path = L"C:\\Users\\Admin\\Documents\\GitHub\\MINE\\Kernel-Cheat-Base\\Build\\Release\\kernel.sys";
	if ( !Utils::ReadFileToMemory( driver_path, &raw_image ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return false;
	}

	Mapper::MapWorkerDriver( iqvw64e_device_handle, raw_image.data( ), &Context::Comms );

	if ( !intel_driver::Unload( iqvw64e_device_handle ) )
		return false;
	
	printf( "[+] initialised!\n" );
	return true;
}

int main( void ) {
	if ( !Initialise( ) )
		return -1;

    std::cout << "PRE: " << da << std::endl;
    Memory::Write( &da, &da2, 4 );
    std::cout << "POST: " << da << std::endl;

    Sleep( 10000 );
    return 0;
}