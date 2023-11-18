#include "esp.h"

void CVisuals::Main( Overlay::CDrawer* d ) {
	if ( !Game::m_pBufferList )
		return;

	for ( unsigned int i{ }; i < Game::m_pBufferList->Count( ); ++i ) {
		const auto object{ Game::m_pBufferList->m_pObjectList->Get( i ) };
		const auto objectName{ object->GetClassName( ) };

		if ( objectName.find( "Scientist" ) != std::string::npos ) {
			printf( "found scientist.\n" );
		}
	}
}