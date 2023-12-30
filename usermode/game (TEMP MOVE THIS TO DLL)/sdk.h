#pragma once
#include "../utils/memory.h"
#include "../../xorstr.h"
#include "../sdk/windows/ntstructs.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>


#undef GetClassName


namespace Game {

	inline bool Init( ) {
		printf( "pasting begins.\n" );
		void* baseAddress{ };
		while ( !Memory::GetGameBaseAddress( &baseAddress ) ) { printf( "L.\n" ); Sleep( 2000 ); }

		printf( "found base address.\n" );

		void* UWORLD{ Memory::Read< void* >( ADD_TO_ADDRESS( baseAddress, 0x11781328 ) ) };
		while ( !UWORLD ) { 
			UWORLD = Memory::Read< void* >( ADD_TO_ADDRESS( baseAddress, 0x11781328 ) ); 
			printf( "L.\n" ); Sleep( 2000 ); 
		}

		if ( !UWORLD )
			return false;
		
		printf( "found UWORLD.\n" );

		printf( "finished initialisation!\n" );

		return true;
	}
}