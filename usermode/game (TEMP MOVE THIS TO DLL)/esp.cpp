#include "esp.h"

bool world_to_screen( const Vector& entity_pos, Vector2D& screen_pos )
{
	auto view_matrix = Game::GetViewMatrix( );
	Vector trans_vec{ view_matrix._14, view_matrix._24, view_matrix._34 };
	Vector right_vec{ view_matrix._11, view_matrix._21, view_matrix._31 };
	Vector up_vec{ view_matrix._12, view_matrix._22, view_matrix._32 };

	float w = trans_vec.Dot( entity_pos ) + view_matrix._44;
	if ( w < 0.098f )
		return false;
	float y = up_vec.Dot( entity_pos ) + view_matrix._42;
	float x = right_vec.Dot( entity_pos ) + view_matrix._41;
	screen_pos = Vector2D( ( 1920 / 2 ) * ( 1.f + x / w ), ( 1080 / 2 ) * ( 1.f - y / w ) );
	return true;
}

void CVisuals::Main( Overlay::CDrawer* d ) {
	Vector2D screen;
	world_to_screen( Vector( 30, 10, 20 ), screen );

	d->Circle( screen, 10, Color( 255, 0, 0 ) );

	if ( !Game::m_pBufferList )
		return;

	static bool once{ };
	if ( !once ) {
		printf( "starting. %d\n", Game::m_pBufferList->Count( ) );
		once = true;
	}

	for ( int i{ }; i < Game::m_pBufferList->Count( ); ++i ) {
		const auto obj{ Game::m_pBufferList->m_pObjectList->Get( i ) };
		if ( !obj )
			continue;

		const auto objectName{ obj->GetClassName( ) };


		//std::cout << objectName << std::endl;

		if ( objectName.find( "BasePlayer" ) != std::string::npos ) {
			/*auto base_object = Memory::Read( ADD_TO_ADDRESS( obj, 0x10 ) );
			auto object = Memory::Read( ADD_TO_ADDRESS( base_object, 0x30 ) );

			if ( i == 0 )
				Game::m_pLocal = object;

			auto tag = Memory::Read< WORD >( ADD_TO_ADDRESS( object, 0x54 ) );
			if ( tag != 6 ) 
				continue;

			auto object_class = Memory::Read( ADD_TO_ADDRESS( object, 0x30 ) );
			auto entity = Memory::Read( ADD_TO_ADDRESS( object_class, 0x18 ) );
			auto transform = Memory::Read( ADD_TO_ADDRESS( object_class, 0x8 ) );
			auto visual_state = Memory::Read( ADD_TO_ADDRESS( transform, 0x38 ) );
			auto object_location = Memory::Read< Vector >( ADD_TO_ADDRESS( visual_state, 0x90 ) );*/

			printf( "found player.\n" );

			const auto pos{ Memory::Read<Vector>( ADD_TO_ADDRESS( obj->GetPositionComponent( ), 0x90 ) ) };
			if ( !pos.IsValid( ) )
				continue;

			Vector2D screen2d;
			if ( !world_to_screen( pos, screen2d ) )
				continue;

			d->Circle( screen2d, 10, Color( 255, 255, 255 ) );
		}
	}
}

/*for ( int i{ }; i < Game::m_pPlayerList->Count( ); ++i ) {
const auto entity{ Game::m_pPlayerList->m_pObjectList->Get( i ) };
if ( !entity )
continue;

if ( i == 0 )
Game::m_pLocal = entity;

printf( "found object.\n" );

const auto objectName{ entity->GetClassName( ) };

std::cout << objectName << std::endl;

/*if ( objectName.find( "Scientist" ) != std::string::npos
	|| objectName.find( "StashContai" ) != std::string::npos ) {
	//printf( "found scientist.\n" );

	const auto pos{ Memory::Read<Vector>( ADD_TO_ADDRESS( object->GetPositionComponent( ), 0x90 ) ) };
	if ( !pos.IsValid( ) )
		continue;

	Vector2D screen2d;
	if ( !world_to_screen( pos, screen2d ) )
		continue;

	d->Circle( screen2d, 10, Color( 255, 255, 255 ) );
}
	}*/