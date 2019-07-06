#pragma once

#include "gameobject.h"

class CFryupZone : public CGameObject {
	typedef	CGameObject	inherited;

public:
	CFryupZone	();
	virtual			~CFryupZone	();

#ifdef DEBUG
	virtual void	OnRender				( );
#endif

};