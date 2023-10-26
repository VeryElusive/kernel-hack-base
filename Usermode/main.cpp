#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "mapper/mapper.h"
#include "ext/intel_driver.h"

#include "game (TEMP MOVE THIS TO DLL)/esp.h"

int da2{ 420 };
int da{ 0 };

HANDLE iqvw64e_device_handle;

void Initialise( ) {
	Context::Comms.m_pGameProcessId = GetCurrentProcessId( );
	Context::Comms.m_pClientProcessId = GetCurrentProcessId( );
	//Context::Comms.m_iSignage = 0xFADED;
	Context::Comms.m_pBuffer = &Context::CommunicationBuffer;

	iqvw64e_device_handle = intel_driver::Load( );
	if ( iqvw64e_device_handle == INVALID_HANDLE_VALUE )
		return;

	std::vector<uint8_t> raw_image = { 0 };
	const std::wstring driver_path = L"kernel.sys";// remember to make this load it off server
	if ( !Utils::ReadFileToMemory( driver_path, &raw_image ) ) {
		intel_driver::Unload( iqvw64e_device_handle );
		return;
	}

	Mapper.MapWorkerDriver( iqvw64e_device_handle, raw_image.data( ), &Context::Comms );

	// TODO: remove from here and put back after init
	intel_driver::Unload( iqvw64e_device_handle );
	printf( "unloaded driver!\n" );
}

void __cdecl VisualCallback( Overlay::CDrawer* d ) {
	Features::Visuals.Main( d );

	d->RoundedRectFilled( { 300,100 }, { 200,100 }, Color( 255, 90, 180 ), 5.f );

	d->RectFilled( { 0,100 }, { 200,100 }, Color( 100, 255, 255 ) );
}

int main( void ) {
	//std::thread init( Initialise );
	//init.detach( );

    //std::cout << "PRE: " << da << std::endl;
    //Memory::Write( &da, &da2, 4 );
    //std::cout << "POST: " << da << std::endl;

	//intel_driver::Unload( iqvw64e_device_handle );

	Overlay::CDrawer d{ Overlay::CreateOverlayWindow( ), FindWindowA( NULL, "Steam" ) };

	std::thread overlay{ Overlay::Main, &d };
	overlay.detach( );

	Overlay::m_pVisualCallback = VisualCallback;

	//LoadCheatModule( Overlay::m_pVisualCallback );

	if ( !Game::Init( ) )
		goto END;

	while ( true ) { };

END:
	Memory::UnloadDriver( );

    return 0;
}