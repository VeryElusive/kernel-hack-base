#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "mapper/mapper.h"
#include "ext/intel_driver.h"

#include "game (TEMP MOVE THIS TO DLL)/esp.h"

#include <TlHelp32.h>
#include <string>

void Initialise( ) {
	Context::Comms.m_pClientProcessId = GetCurrentProcessId( );
	//Context::Comms.m_iSignage = 0xFADED;
	Context::Comms.m_pBuffer = &Context::CommunicationBuffer;

	intel_driver::iqvw64e_device_handle = intel_driver::Load( );
	if ( intel_driver::iqvw64e_device_handle == INVALID_HANDLE_VALUE ) {
		printf( "failed to init vuln driver\n" );
		return;
	}

	std::vector<uint8_t> raw_image = { 0 };
	const std::wstring driver_path = L"kernel.sys";// remember to make this load it off server
	if ( !Utils::ReadFileToMemory( driver_path, &raw_image ) ) {
		intel_driver::Unload( intel_driver::iqvw64e_device_handle );
		return;
	}

	Mapper.MapWorkerDriver( intel_driver::iqvw64e_device_handle, raw_image.data( ), &Context::Comms );

	Sleep( 5000 );
	
	Context::ReadyToClose = true;
}

void __cdecl VisualCallback( Overlay::CDrawer* d ) {
	//Features::Visuals.Main( d );

	d->RoundedRectFilled( { 300,100 }, { 200,100 }, Color( 255, 90, 180 ), 5.f );

	d->RectFilled( { 0,100 }, { 200,100 }, Color( 100, 255, 255 ) );
}


int main( ) {
	std::thread init( Initialise );
	init.detach( );

	Memory::WaitForDriver( );

	printf( "pass!\n" );

	printf( "open game now\n" );

	intel_driver::Unload( intel_driver::iqvw64e_device_handle );

	/* open game now */
	Memory::WaitForGame( xors( L"Dbgview.exe" ) );

	printf( "lessgo\n" );
	Sleep( 5000 );

	//Overlay::CDrawer d{ Overlay::CreateOverlayWindow( ), FindWindowA( NULL, "Rust" ) };

	//std::thread overlay{ Overlay::Main, &d };
	//overlay.detach( );

	Overlay::m_pVisualCallback = VisualCallback;

	//LoadCheatModule( Overlay::m_pVisualCallback );

	//if ( !Game::Init( ) )
	//	goto END;

	//while ( true ) { };*/


END:
	Memory::UnloadDriver( );

	while ( !Context::ReadyToClose ) { }

	exit( 1 );

    return 0;
}