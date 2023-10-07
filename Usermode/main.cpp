#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "utils/memory.h"

int da2{ 420 };
int da{ 0 };

PVOID( NTAPI* NtConvertBetweenAuxiliaryCounterAndPerformanceCounter )( PVOID, PVOID, PVOID, PVOID );

void KernelThread( PVOID LParam ) {
	INT64 Status{ 0 };

	CommsParse_t Data{ *( CommsParse_t* ) LParam };
	PVOID pData{ &Data };

	HMODULE Module{ LoadLibrary( L"ntdll.dll" ) };

	if ( !Module ) {
		return;
	}

	*( PVOID* ) &NtConvertBetweenAuxiliaryCounterAndPerformanceCounter = GetProcAddress( Module, "NtConvertBetweenAuxiliaryCounterAndPerformanceCounter" );

	if ( !NtConvertBetweenAuxiliaryCounterAndPerformanceCounter ) {
		return;
	}
	// Endless function, so if it success, there's an error
	NtConvertBetweenAuxiliaryCounterAndPerformanceCounter( ( PVOID ) 1, &pData, &Status, nullptr );
	printf( "error! [nn]" );
}

int main( void ) {
    Context::Comms.m_pGameProcessId = GetCurrentProcessId( );
    Context::Comms.m_pClientProcessId = GetCurrentProcessId( );
    Context::Comms.m_iSignage = 0xFADED;
    Context::Comms.m_pBuffer = &Context::CommunicationBuffer;

	CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE ) KernelThread, &Context::Comms, 0, nullptr );

    std::cout << "PRE: " << da << std::endl;
    Memory::Write( &da, &da2, 4 );
    std::cout << "POST: " << da << std::endl;

    Sleep( 10000 );
    return 0;
}