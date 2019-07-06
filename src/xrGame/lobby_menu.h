////////////////////////////////////////////////////////////////////////////
//	Created		: 10.01.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef LOBBY_MENU_H_INCLUDED
#define LOBBY_MENU_H_INCLUDED



#include "../xrEngine/IInputReceiver.h"
#include "../xrEngine/IGame_Persistent.h"
#include "UIDialogHolder.h"
#include "ui/UIWindow.h"
#include "ui/UIWndCallback.h"
#include "ui_base.h"

class ui_lobby_dialog;

class lobby_menu :
	public IMainMenu,
	public IInputReceiver,
	public pureRender,
	public CDialogHolder,
	public CUIWndCallback,
	public CUIWindow,
	public CDeviceResetNotifier

{
	ui_lobby_dialog*		m_startDialog;
	

	enum{
		flRestoreConsole	= (1<<0),
		flRestorePause		= (1<<1),
		flActive			= (1<<3),
		flNeedChangeCapture	= (1<<4),
		flRestoreCursor		= (1<<5),
		flNeedVidRestart	= (1<<7),
	};
	Flags16			m_Flags;
	xr_vector<CUIWindow*>				m_pp_draw_wnds;

public:
	u32				m_start_time;

	shared_str		m_player_name;
	bool			ReloadUI						( );

public:
	void			RegisterPPDraw					(CUIWindow* w);
	void			UnregisterPPDraw				(CUIWindow* w);

public:
	u32				m_deactivated_frame;
	bool			m_activatedScreenRatio;

	virtual void	DestroyInternal					(bool bForce);
					lobby_menu						();
	virtual			~lobby_menu						();

	virtual void	Activate						(bool bActive); 
	virtual	bool	IsActive						(); 
	virtual	bool	CanSkipSceneRendering			(); 

	virtual bool	IgnorePause						()	{return true;}


	virtual void	IR_OnMousePress					(int btn);
	virtual void	IR_OnMouseRelease				(int btn);
	virtual void	IR_OnMouseHold					(int btn);
	virtual void	IR_OnMouseMove					(int x, int y);
	virtual void	IR_OnMouseStop					(int x, int y);

	virtual void	IR_OnKeyboardPress				(int dik);
	virtual void	IR_OnKeyboardRelease			(int dik);
	virtual void	IR_OnKeyboardHold				(int dik);

	virtual void	IR_OnMouseWheel					(int direction)	;

	bool			OnRenderPPUI_query				();
	void			OnRenderPPUI_main				();
	void			OnRenderPPUI_PP					();

	virtual void			OnRender						();
	virtual void	_BCL	OnFrame							(void);

	virtual bool	UseIndicators					()						{return false;}

	void			OnDeviceCreate					();

	void			SetNeedVidRestart				();
	virtual void	OnDeviceReset					();

	LPCSTR			GetPlayerName					();

public:
	// just for callback support
	virtual void	SendMessage						(CUIWindow* pWnd, s16 msg, void* pData = NULL);
	void		xr_stdcall		on_btn_figth_clicked		(CUIWindow* w, void* d);
	void		xr_stdcall		on_btn_login_clicked		(CUIWindow* w, void* d);
};

extern lobby_menu*	MainMenu();

#endif // #ifndef LOBBY_MENU_H_INCLUDED