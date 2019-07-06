////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_XRENGINE_D3DCONSOLE_H_INCLUDED
#define ENGINE_XRENGINE_D3DCONSOLE_H_INCLUDED

#include "xr_ioConsole.h"

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/UIShader.h"

class ENGINE_API CGameFont;

class CD3DConsole:	public CConsole, 
					public pureRender,
					public pureScreenResolutionChanged
{
	typedef CConsole inherited;
public:
	virtual void		OnRender					( );
	virtual void		OnScreenResolutionChanged	( );
	virtual	void		Initialize					( );
	virtual	void		Destroy						( );

	virtual void		Show				( );
	virtual void		Hide				( );

						CD3DConsole					( );
	virtual				~CD3DConsole				( );
protected:
	CGameFont*		pFont;
	CGameFont*		pFont2;
	
	FactoryPtr<IUIShader>*		m_hShader_back;

	void	OutFont				( LPCSTR text, float& pos_y );
	void	DrawBackgrounds		( bool bGame );
	void	DrawRect			( Frect const& r, u32 color );

};

#endif // #ifndef ENGINE_XRENGINE_D3DCONSOLE_H_INCLUDED