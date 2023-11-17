#pragma once
#include "../utils/memory.h"
#include "../../xorstr.h"
#include "../sdk/windows/ntstructs.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>

#define ADD_TO_ADDRESS( addr, offset )reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( addr ) + offset )

class CObjectList {
public:
	void* Get( int i ) {
		return Memory::Read< void* >( ADD_TO_ADDRESS( this, ( 0x20 + ( i * 0x8 ) ) ) );
	}
};

// NOTES: System::Object is sized at 0x10. 
// Type: BufferList1, which is an overload of System::Object
class CBufferList {
public:
	inline static CObjectList* m_pObjectList;

	uint32_t Count( ) { 
		return Memory::Read< uint32_t >( ADD_TO_ADDRESS( this, 0x10 ) ); 
	};
};

namespace Game {
	inline CBufferList* m_pBufferList{ };
	inline bool Init( ) {
		void* moduleBase{ Memory::GetModuleBase( xors( L"GameAssembly.dll" ) ) };
		while ( !moduleBase ) {
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			moduleBase = Memory::GetModuleBase( xors( L"GameAssembly.dll" ) );
		}

		if ( moduleBase )
			std::cout << "found GameAssembly.dll." << std::endl;

		// Object name: BaseNetworkable_TypeInfo
		// Type: BaseNetworkable_c
		//void* baseNetworkable{ }; Memory::Read( ADD_TO_ADDRESS( moduleBase, 0x333CBC8 ), &baseNetworkable, 8 );
		void* baseNetworkable{ Memory::Read< void* >( ADD_TO_ADDRESS( moduleBase, 0x333CBC8 ) ) };
		if ( !baseNetworkable )
			return false;

		std::cout << "found baseNetworkable." << std::endl;

		// literally just all the static fields, lol
		void* staticFields{ Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) ) };
		if ( !staticFields )
			return false;

		std::cout << "found staticFields." << std::endl;

		// Type: EntityRealm
		void* clientEntities{ Memory::Read< void* >( staticFields ) };
		if ( !clientEntities )
			return false;

		std::cout << "found clientEntities." << std::endl;

		// Type: ListDictionary2 (which is an overload of ListDictionary)
		void* entityAndKeys{ Memory::Read< void* >( ADD_TO_ADDRESS( clientEntities, 0x10 ) ) };
		if ( !entityAndKeys )
			return false;

		std::cout << "found entityAndKeys." << std::endl;

		// Type: BufferList1
		m_pBufferList = Memory::Read< CBufferList* >( ADD_TO_ADDRESS( entityAndKeys, 0x28 ) );
		if ( !m_pBufferList )
			return false;	

		std::cout << "found m_pBufferList." << std::endl;
		
		CBufferList::m_pObjectList = Memory::Read< CObjectList* >( ADD_TO_ADDRESS( entityAndKeys, 0x28 ) );
		if ( !CBufferList::m_pObjectList )
			return false;

		std::cout << "found CBufferList::m_pObjectList." << std::endl;

		return true;
	}
}