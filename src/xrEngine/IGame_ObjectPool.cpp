#include "stdafx.h"
#include "igame_objectpool.h"
#include "xr_object.h"

CObject* Game_ObjectPool::create( LPCSTR name )
{
	CLASS_ID CLS		= pSettings->r_clsid		(name,"class");
	CObject* O			= (CObject*) NEW_INSTANCE	(CLS);
	O->cNameSect_set	(name);
	O->Load				(name);
	return				O;
}

void Game_ObjectPool::destroy( CObject* O )
{
	xr_delete				(O);
}
