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
	if ( !Game::m_pBufferList )
		return;

	for ( unsigned int i{ }; i < Game::m_pBufferList->Count( ); ++i ) {
		const auto object{ Game::m_pBufferList->m_pObjectList->Get( i ) };
		const auto objectName{ object->GetClassName( ) };

		if ( objectName.find( "Scientist" ) != std::string::npos
			|| objectName.find( "StashContai" ) != std::string::npos ) {
			printf( "found scientist.\n" );

			const auto pos{ Memory::Read<Vector>( ADD_TO_ADDRESS( object->GetPositionComponent( ), 0x90 ) ) };
			if ( !pos.IsValid( ) )
				continue;

			Vector2D screen2d;
			if ( !world_to_screen( pos, screen2d ) )
				continue;

			d->Circle( screen2d, 10, Color( 255, 255, 255 ) );
		}
	}
}