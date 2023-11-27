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

	int Count( ) {
		return Memory::Read< int >( ADD_TO_ADDRESS( this, 0x10 ) );
	};
};

class CGameObjectManager {
public:
	
};

namespace Game {
	inline CBufferList* m_pBufferList{ };
	//inline CGameObjectManager* m_pGameObjectManager{ };
	inline void* m_pCameraInstance{ };
	inline CBufferList* m_pPlayerList{ };
	inline void* m_pLocal{ };

	inline Matrix4x4_t GetViewMatrix( ) {
		return Memory::Read<Matrix4x4_t>( ADD_TO_ADDRESS( m_pCameraInstance, 0x2E4 ) );
	}

	inline bool Init( ) {
		printf( "searching for ga\n" );
		void* gameAssembly{ Memory::GetModuleBase( xors( L"GameAssembly.dll" ) ) };
		while ( !gameAssembly ) {
			printf( "fail\n" );
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			gameAssembly = Memory::GetModuleBase( xors( L"GameAssembly.dll" ) );
		}

		printf( "found ga\n" );
		
		/*void* unityPlayer{ Memory::GetModuleBase( xors( L"UnityPlayer.dll" ) ) };
		while ( !unityPlayer ) {
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			unityPlayer = Memory::GetModuleBase( xors( L"UnityPlayer.dll" ) );
		}*/

		Sleep( 2000 );

		// Object name: BaseNetworkable_TypeInfo
		// Type: BaseNetworkable_c
		void* baseNetworkable{ Memory::Read< void* >( ADD_TO_ADDRESS( gameAssembly, 0x333CBD8 ) ) };
		if ( !baseNetworkable )
			return false;

		printf( "1\n" );
		
		// prob move to this soon
		//void* entityAndKeys = Memory::ReadChain( gameAssembly, { 0x333CBC8, 0xB8, 0x10 } );

		/* this only exists when in game */

		// TODO: check local plyer connected
		/*"Name": "LocalPlayer_TypeInfo",
			"Signature" : "LocalPlayer_c*"*/

		// literally just all the static fields, lol
		void* staticFields{ Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) ) };

		while ( !staticFields ) {
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
			staticFields = Memory::Read< void* >( ADD_TO_ADDRESS( baseNetworkable, 0xB8 ) );
		}

		printf( "found sf\n" );

		// Type: EntityRealm
		void* clientEntities{ Memory::Read< void* >( staticFields ) };
		if ( !clientEntities )
			return false;

		printf( "2\n" );

		// Type: ListDictionary2 (which is an overload of ListDictionary)
		void* entityAndKeys{ Memory::Read< void* >( ADD_TO_ADDRESS( clientEntities, 0x10 ) ) };
		if ( !entityAndKeys )
			return false;

		printf( "3\n" );

		// Type: BufferList1
		m_pBufferList = Memory::Read< CBufferList* >( ADD_TO_ADDRESS( entityAndKeys, 0x28 ) );
		if ( !m_pBufferList )
			return false;	

		printf( "4\n" );

		CBufferList::m_pObjectList = Memory::Read< CObjectList* >( ADD_TO_ADDRESS( m_pBufferList, 0x18 ) );
		if ( !CBufferList::m_pObjectList )
			return false;

		printf( "1!\n" );

		//https://i.epvpimg.com/lnxDgab.png

		// Object name: MainCamera_TypeInfo
		// Type: MainCamera_c
		void* MainCamera_TypeInfo{ Memory::Read( ADD_TO_ADDRESS( gameAssembly, 54172280 ) ) };
		if ( !MainCamera_TypeInfo )
			return false;

		printf( "2!\n" );

		void* camera_staticfields { Memory::Read( ADD_TO_ADDRESS( MainCamera_TypeInfo, 0xB8 ) ) };
		if ( !camera_staticfields )
			return false;

		printf( "3!\n" );

		void* main_camera{ Memory::Read( camera_staticfields ) };
		if ( !main_camera )
			return false;

		printf( "4!\n" );

		m_pCameraInstance = Memory::Read( ADD_TO_ADDRESS( main_camera, 0x10 ) );
		if ( !m_pCameraInstance )
			return false;

		printf( "finished initialisation!\n" );

		/*"Address": 53727248,
		  "Name": "BasePlayer_TypeInfo",
		  "Signature": "BasePlayer_c*"

		static UINT64 oPlayerList1 = 0;
		const auto BasePlayer_TypeInfo = Memory::Read( ADD_TO_ADDRESS( gameAssembly, 53727248 ) );
		if ( !BasePlayer_TypeInfo )
			return false;

		void* basePlayerActual = Memory::Read( ADD_TO_ADDRESS( BasePlayer_TypeInfo, 0xB8 ) );
		if ( !basePlayerActual )
			return false;

		void* visiblePlayerList = Memory::Read( ADD_TO_ADDRESS( basePlayerActual, 0x20 ) ); // i saw this as 0x8/0x10 online, i found 0x20 on dnspy tho
		if ( !visiblePlayerList )
			return false;

		// visiblePlayerList->vals
		m_pPlayerList = Memory::Read< CBufferList* >( ADD_TO_ADDRESS( visiblePlayerList, 0x28 ) );
		if ( !m_pPlayerList )
			return false;*/

		return true;
	}
}