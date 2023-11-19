#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "mapper/mapper.h"
#include "ext/intel_driver.h"

#include "game (TEMP MOVE THIS TO DLL)/esp.h"

#include <TlHelp32.h>
#include <string>

LONG WINAPI SimplestCrashHandler( EXCEPTION_POINTERS* ExceptionInfo ) {
	UNREFERENCED_PARAMETER( ExceptionInfo );

	if ( intel_driver::iqvw64e_device_handle )
		intel_driver::Unload( intel_driver::iqvw64e_device_handle );

	printf( "crashed.\n" );
	Sleep( 5000 );

	return EXCEPTION_EXECUTE_HANDLER;
}


void Initialise( ) {
	Context::Comms.m_pClientProcessId = ( HANDLE ) GetCurrentProcessId( );
	//Context::Comms.m_pBuffer = &Context::CommunicationBuffer;
	Context::Comms.m_pBuffer = const_cast< DataRequest_t* >( &Context::CommunicationBuffer );


	intel_driver::iqvw64e_device_handle = intel_driver::Load( );
	if ( intel_driver::iqvw64e_device_handle == INVALID_HANDLE_VALUE ) {
		printf( "failed to init vuln driver\n" );
		return;
	}

	std::vector<uint8_t> raw_image = { 0 };
	const std::wstring driver_path = L"kernel.sys";
	if ( !Utils::ReadFileToMemory( driver_path, &raw_image ) ) {
		intel_driver::Unload( intel_driver::iqvw64e_device_handle );
		return;
	}

	// remember to make this load it off server
	Mapper.MapWorkerDriver( intel_driver::iqvw64e_device_handle, raw_image.data( ), &Context::Comms );


	//Sleep( 5000 );

	//Context::Close = true;

	printf( "exiting." );

	std::exit( 0 );
}

void __cdecl VisualCallback( Overlay::CDrawer* d ) {
	Features::Visuals.Main( d );

	d->RoundedRectFilled( { 300,100 }, { 200,100 }, Color( 255, 90, 180 ), 5.f );

	d->RectFilled( { 0,100 }, { 200,100 }, Color( 100, 255, 255 ) );
}

void CloseApplication( ) {
	Memory::UnloadDriver( );

	//intel_driver::Unload( intel_driver::iqvw64e_device_handle );
	//Sleep( 3000 );

	while ( true ) { };

	//while ( !Context::Close ) { };

	//std::exit( 0 );  // Terminate the program
}

BOOL WINAPI ConsoleHandler( DWORD eventType ) {
	if ( eventType == CTRL_CLOSE_EVENT ) {
		CloseApplication( );
		return FALSE;
	}

	// Return TRUE for events that are handled, FALSE otherwise
	return TRUE;
}

int main( ) {
	SetUnhandledExceptionFilter( SimplestCrashHandler );

	if ( !SetConsoleCtrlHandler( ConsoleHandler, TRUE ) )
		return 1;

	std::thread init( Initialise );
	init.detach( );

	printf( "waiting.\n" );

	Memory::WaitForDriver( );

	printf( "pass!\n" );

	intel_driver::Unload( intel_driver::iqvw64e_device_handle );

	printf( "open game now\n" );

	//intel_driver::Unload( intel_driver::iqvw64e_device_handle );

	/* open game now */
	//Memory::WaitForGame( xors( "RustClient.exe" ) );

	Overlay::CDrawer d{ Overlay::CreateOverlayWindow( ), FindWindowA( NULL, "Rust" ) };

	std::thread overlay{ Overlay::Main, &d };
	overlay.detach( );

	Overlay::m_pVisualCallback = VisualCallback;

	//LoadCheatModule( Overlay::m_pVisualCallback );

	//if ( !Game::Init( ) )
	//	printf( "fail\n" );
	//else
	//	printf( "w chat\n" );

	printf( "You can close this window now.\n" );

	do {

	}
	while ( true ); 

    return 0;
}