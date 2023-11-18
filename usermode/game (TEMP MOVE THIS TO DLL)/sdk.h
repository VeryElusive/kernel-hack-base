#pragma once
#include "../utils/memory.h"
#include "../../xorstr.h"
#include "../sdk/windows/ntstructs.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>

#define ADD_TO_ADDRESS( addr, offset )reinterpret_cast< void* >( reinterpret_cast< uintptr_t >( addr ) + offset )

#undef GetClassName

class CObject {
public:
	std::string GetClassName( ) {
		const auto object_unk{ Memory::Read<uintptr_t>( this ) };
		if ( !object_unk )
			return {};

		std::unique_ptr<char[ ]> buffer( new char[ 13 ] );
		Memory::Read( Memory::Read( reinterpret_cast< void* >( object_unk + 0x10 ) ), buffer.get( ), 13 );
		return std::string( buffer.get( ) );
	}
};

class CObjectList {
public:
	CObject* Get( int i ) {
		return Memory::Read< CObject* >( ADD_TO_ADDRESS( this, ( 0x20 + ( i * 0x8 ) ) ) );
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

		/* this only exists when in game */

		// TODO: check local plyer connected
		/*"Address": 54160440,
		"Name": "LocalPlayer_TypeInfo",
			"Signature" : "LocalPlayer_c*"*/

		// literally just all the static fields, lol
		void* staticFields{ Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) ) };

		while ( !staticFields )
			staticFields = Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) );

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