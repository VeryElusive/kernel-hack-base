#pragma once
#include "../utils/memory.h"
#include "../../xorstr.h"
#include "../sdk/windows/ntstructs.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>


#undef GetClassName

class CObject {
public:
	std::string GetClassName( ) {
		const auto object_unk{ Memory::Read<void*>( this ) };
		if ( !object_unk )
			return {};

		std::unique_ptr<char[ ]> buffer( new char[ 13 ] );
		Memory::Read( Memory::Read( ADD_TO_ADDRESS( object_unk, 0x10 ) ), buffer.get( ), 13 );
		return std::string( buffer.get( ) );
	}

	void* GetPositionComponent( ) {
		const auto player_visual{ Memory::Read<void*>( ADD_TO_ADDRESS( this, 0x8 ) ) };
		if ( !player_visual )
			return NULL;

		return Memory::Read( ADD_TO_ADDRESS( player_visual, 0x38 ) );
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

class CGameObjectManager {
public:
	
};

namespace Game {
	inline CBufferList* m_pBufferList{ };
	inline CGameObjectManager* m_pGameObjectManager{ };
	inline void* m_pCameraInstance{ };

	inline Matrix4x4_t GetViewMatrix( ) {
		return Memory::Read<Matrix4x4_t>( ADD_TO_ADDRESS( m_pCameraInstance, 0xDC ) );
	}

	inline bool Init( ) {
		void* gameAssembly{ Memory::GetModuleBase( xors( L"GameAssembly.dll" ) ) };
		while ( !gameAssembly ) {
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			gameAssembly = Memory::GetModuleBase( xors( L"GameAssembly.dll" ) );
		}		
		
		void* unityPlayer{ Memory::GetModuleBase( xors( L"UnityPlayer.dll" ) ) };
		while ( !unityPlayer ) {
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			unityPlayer = Memory::GetModuleBase( xors( L"UnityPlayer.dll" ) );
		}

		// Object name: BaseNetworkable_TypeInfo
		// Type: BaseNetworkable_c
		//void* baseNetworkable{ }; Memory::Read( ADD_TO_ADDRESS( gameAssembly, 0x333CBC8 ), &baseNetworkable, 8 );
		void* baseNetworkable{ Memory::Read< void* >( ADD_TO_ADDRESS( gameAssembly, 0x333CBC8 ) ) };
		if ( !baseNetworkable )
			return false;

		/* this only exists when in game */

		// TODO: check local plyer connected
		/*"Address": 54160440,
		"Name": "LocalPlayer_TypeInfo",
			"Signature" : "LocalPlayer_c*"*/

		// literally just all the static fields, lol
		void* staticFields{ Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) ) };

		while ( !staticFields )
			staticFields = Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) );

		// Type: EntityRealm
		void* clientEntities{ Memory::Read< void* >( staticFields ) };
		if ( !clientEntities )
			return false;

		// Type: ListDictionary2 (which is an overload of ListDictionary)
		void* entityAndKeys{ Memory::Read< void* >( ADD_TO_ADDRESS( clientEntities, 0x10 ) ) };
		if ( !entityAndKeys )
			return false;

		// Type: BufferList1
		m_pBufferList = Memory::Read< CBufferList* >( ADD_TO_ADDRESS( entityAndKeys, 0x28 ) );
		if ( !m_pBufferList )
			return false;	

		CBufferList::m_pObjectList = Memory::Read< CObjectList* >( ADD_TO_ADDRESS( entityAndKeys, 0x28 ) );
		if ( !CBufferList::m_pObjectList )
			return false;

		/* unity player */

		printf( "unityplayer!\n" );

		m_pGameObjectManager = Memory::Read<CGameObjectManager*>( ADD_TO_ADDRESS( unityPlayer, 0x1AD8580 ) );
		if ( !m_pGameObjectManager )
			return false;

		printf( "1!\n" );

		auto tagged_objects{ Memory::Read( ADD_TO_ADDRESS( m_pGameObjectManager, 0x8 ) ) };
		if ( !tagged_objects )
			return false;

		printf( "2!\n" );

		auto game_object{ Memory::Read( ADD_TO_ADDRESS( tagged_objects, 0x10 ) ) };
		if ( !tagged_objects )
			return false;

		printf( "3!\n" );

		auto object_class{ Memory::Read( ADD_TO_ADDRESS( game_object, 0x30 ) ) };
		if ( !tagged_objects )
			return false;

		printf( "4!\n" );

		m_pCameraInstance = Memory::Read( ADD_TO_ADDRESS( object_class, 0x18 ) );
		if ( !m_pCameraInstance )
			return false;

		printf( "finished initialisation!\n" );

		return true;
	}
}