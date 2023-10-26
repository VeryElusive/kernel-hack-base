#pragma once
#include "../overlay/overlay.h"
#include "sdk.h"

class CVisuals {
public:

	void Main( Overlay::CDrawer* d );
};

namespace Features { inline CVisuals Visuals; };