////////////////////////////////////////////////////////////////////////////
//	Created		: 10.01.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "lobby_menu.h"
#include "ui_lobby_dialog.h"
#include "ui/UIBtnHint.h"
#include "ui/UIXmlInit.h"
#include "UICursor.h"
#include "../xrEngine/xr_ioc_cmd.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/CameraManager.h"
#include <dinput.h>

lobby_menu*	MainMenu()	{return (lobby_menu*)g_pGamePersistent->m_pMainMenu; };

lobby_menu::lobby_menu( )
{
	m_Flags.zero					();
	m_startDialog					= NULL;
	g_pGamePersistent->m_pMainMenu	= this;
	if (Device.b_is_Ready)			OnDeviceCreate();  	
	CUIXmlInit::InitColorDefs		();
	g_btnHint						= NULL;
	g_statHint						= NULL;
	m_start_time					= 0;
	g_btnHint						= xr_new<CUIButtonHint>();
	g_statHint						= xr_new<CUIButtonHint>();

	Device.seqFrame.Add		(this,REG_PRIORITY_LOW-1000);
}

lobby_menu::~lobby_menu( )
{
	Device.seqFrame.Remove			(this);
	xr_delete						(g_btnHint);
	xr_delete						(g_statHint);
	xr_delete						(m_startDialog);
	g_pGamePersistent->m_pMainMenu	= NULL;
}

void lobby_menu::Activate	(bool bActivate)
{
	if (	!!m_Flags.test(flActive) == bActivate)		return;

	bool b_is_single				= false;

	if(bActivate)
	{
		Device.Pause				(TRUE, FALSE, TRUE, "mm_activate1");
		m_Flags.set					(flActive|flNeedChangeCapture,TRUE);

		m_Flags.set					(flRestoreCursor,GetUICursor().IsVisible());

		if(!ReloadUI())				return;

		m_Flags.set					(flRestoreConsole,pConsole->bVisible);

		if(b_is_single)	m_Flags.set	(flRestorePause,Device.Paused());

		pConsole->Hide				();


		if(g_pGameLevel)
		{
			if(b_is_single){
				Device.seqFrame.Remove		(g_pGameLevel);
			}
			Device.seqRender.Remove			(g_pGameLevel);
			CCameraManager::ResetPP			();
		};
		Device.seqRender.Add				(this, 4); // 1-console 2-cursor 3-tutorial

		pConsoleCommands->Execute					("stat_memory");
	}else{
		m_deactivated_frame					= Device.dwFrame;
		m_Flags.set							(flActive,				FALSE);
		m_Flags.set							(flNeedChangeCapture,	TRUE);

		Device.seqRender.Remove				(this);

		bool b = !!pConsole->bVisible;
		if(b){
			pConsole->Hide					();
		}

		IR_Release							();
		if(b){
			pConsole->Show					();
		}

		if(m_startDialog->IsShown())
			m_startDialog->HideDialog		();

		CleanInternals						();
		if(g_pGameLevel)
		{
			if(b_is_single){
				Device.seqFrame.Add			(g_pGameLevel);

			}
			Device.seqRender.Add			(g_pGameLevel);
		};
		if(m_Flags.test(flRestoreConsole))
			pConsole->Show			();

		if(m_Flags.test(flRestoreCursor))
			GetUICursor().Show			();

		Device.Pause					(FALSE, TRUE, TRUE, "mm_deactivate2");

		if(m_Flags.test(flNeedVidRestart))
		{
			m_Flags.set			(flNeedVidRestart, FALSE);
			pConsoleCommands->Execute	("vid_restart");
		}
	}
}

bool lobby_menu::ReloadUI()
{
	if(m_startDialog)
	{
		if(m_startDialog->IsShown())
			m_startDialog->HideDialog		();
		CleanInternals						();
	}
	xr_delete					(m_startDialog);
	lobby_menu* p = this;
	m_startDialog				= xr_new<ui_lobby_dialog>( p );
	m_startDialog->m_bWorkInPause= true;
	m_startDialog->ShowDialog	(true);

	m_activatedScreenRatio		= (float)Device.dwWidth/(float)Device.dwHeight > (UI_BASE_WIDTH/UI_BASE_HEIGHT+0.01f);
	return true;
}

bool lobby_menu::IsActive()
{
	return !!m_Flags.test(flActive);
}

bool lobby_menu::CanSkipSceneRendering()
{
	return IsActive();
}

//IInputReceiver
static int mouse_button_2_key []	=	{MOUSE_1,MOUSE_2,MOUSE_3};

void lobby_menu::IR_OnMousePress(int btn)	
{	
	if(!IsActive()) return;

	IR_OnKeyboardPress(mouse_button_2_key[btn]);
};

void	lobby_menu::IR_OnMouseRelease(int btn)	
{
	if(!IsActive()) return;

	IR_OnKeyboardRelease(mouse_button_2_key[btn]);
};

void	lobby_menu::IR_OnMouseHold(int btn)	
{
	if(!IsActive()) return;

	IR_OnKeyboardHold(mouse_button_2_key[btn]);

};

void	lobby_menu::IR_OnMouseMove(int x, int y)
{
	if(!IsActive()) return;
	CDialogHolder::IR_UIOnMouseMove(x, y);
};

void	lobby_menu::IR_OnMouseStop(int x, int y)
{
};

void	lobby_menu::IR_OnKeyboardPress(int dik)
{
	if(!IsActive()) return;

	if( is_binded(kCONSOLE, dik) )
	{
		pConsole->Show();
		return;
	}
	if (DIK_F12 == dik){
		Render->Screenshot();
		return;
	}

	CDialogHolder::IR_UIOnKeyboardPress(dik);
};

void	lobby_menu::IR_OnKeyboardRelease			(int dik)
{
	if(!IsActive()) return;
	
	CDialogHolder::IR_UIOnKeyboardRelease(dik);
};

void	lobby_menu::IR_OnKeyboardHold(int dik)	
{
	if(!IsActive()) return;
	
	CDialogHolder::IR_UIOnKeyboardHold(dik);
};

void lobby_menu::IR_OnMouseWheel(int direction)
{
	if(!IsActive()) return;
	
	CDialogHolder::IR_UIOnMouseWheel(direction);
}


bool lobby_menu::OnRenderPPUI_query()
{
	return false; //IsActive() && b_shniaganeed_pp;
}


extern void draw_wnds_rects();
void lobby_menu::OnRender	()
{
	if(g_pGameLevel)
		Render->Calculate			();

	Render->Render				();
	if(!OnRenderPPUI_query())
	{
		DoRenderDialogs();
		UI().RenderFont();
		draw_wnds_rects();
	}
}

void lobby_menu::OnRenderPPUI_main	()
{
	if(!IsActive()) return;


	UI().pp_start();

	if(OnRenderPPUI_query())
	{
		DoRenderDialogs();
		UI().RenderFont();
	}

	UI().pp_stop();
}

void lobby_menu::OnRenderPPUI_PP	()
{
	if ( !IsActive() ) return;


	UI().pp_start();
	
	xr_vector<CUIWindow*>::iterator it = m_pp_draw_wnds.begin();
	for(; it!=m_pp_draw_wnds.end();++it)
	{
		(*it)->Draw();
	}
	UI().pp_stop();
}

//pureFrame
void lobby_menu::OnFrame()
{
	if (m_Flags.test(flNeedChangeCapture))
	{
		m_Flags.set					(flNeedChangeCapture,FALSE);
		if (m_Flags.test(flActive))	IR_Capture();
		else						IR_Release();
	}
	CDialogHolder::OnFrame		();


	if(IsActive())
	{
		bool b_is_16_9	= (float)Device.dwWidth/(float)Device.dwHeight > (UI_BASE_WIDTH/UI_BASE_HEIGHT+0.01f);
		if(b_is_16_9 !=m_activatedScreenRatio)
		{
			ReloadUI();
			m_startDialog->SendMessage(m_startDialog, MAIN_MENU_RELOADED, NULL);
		}
	}
}

void lobby_menu::OnDeviceCreate()
{
}



void lobby_menu::RegisterPPDraw(CUIWindow* w)
{
	UnregisterPPDraw				(w);
	m_pp_draw_wnds.push_back		(w);
}

void lobby_menu::UnregisterPPDraw				(CUIWindow* w)
{
	m_pp_draw_wnds.erase(
		std::remove(
			m_pp_draw_wnds.begin(),
			m_pp_draw_wnds.end(),
			w
		),
		m_pp_draw_wnds.end()
	);
}


void lobby_menu::DestroyInternal(bool bForce)
{
	if(m_startDialog && ((m_deactivated_frame < Device.dwFrame+4)||bForce) )
		xr_delete		(m_startDialog);
}


void lobby_menu::SetNeedVidRestart()
{
	m_Flags.set(flNeedVidRestart,TRUE);
}

void lobby_menu::OnDeviceReset()
{
	if(IsActive() && g_pGameLevel)
		SetNeedVidRestart();
}

LPCSTR lobby_menu::GetPlayerName()
{
	return m_player_name.c_str();
}

void lobby_menu::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent		(pWnd, msg, pData);
}

void lobby_menu::on_btn_figth_clicked(CUIWindow* w, void* d)
{
	//string128 op_client;
	//xr_sprintf( op_client, "%s", m_startDialog->m_login_edit->GetText() );
	LPCSTR op_server = "mp_factory/ah";
	LPCSTR op_client = "localhost/name=dima-ai";
	Engine.Event.Defer( "KERNEL:start",
						u64(xr_strdup(op_server)),
						u64(xr_strdup(op_client)));
}

void lobby_menu::on_btn_login_clicked(CUIWindow* w, void* d)
{
}
