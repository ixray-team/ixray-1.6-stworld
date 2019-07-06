#include "stdafx.h"
#include <dinput.h>
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/xr_ioc_cmd.h"
#include "entity_alive.h"
#include "../xrEngine/fdemorecord.h"
#include "level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "Inventory.h"
#include "xrServer.h"

#include "actor.h"
#include "huditem.h"
#include "UIGameCustom.h"
#include "UI/UIDialogWnd.h"
#include "../xrEngine/xr_input.h"

#include "../Include/xrRender/DebugRender.h"


bool g_bDisableAllInput = false;
extern	float	g_fTimeFactor;

#define CURRENT_ENTITY()	(game?(CurrentControlEntity()):NULL)

void CLevel::IR_OnMouseWheel( int direction )
{
	if(	g_bDisableAllInput	) return;

	if (CurrentGameUI()->IR_UIOnMouseWheel(direction)) return;
	if( Device.Paused()
#ifdef DEBUG
		&& !psActorFlags.test(AF_NO_CLIP) 
#endif //DEBUG
		) return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnMouseWheel(direction);
	}
}

static int mouse_button_2_key []	=	{MOUSE_1,MOUSE_2,MOUSE_3};

void CLevel::IR_OnMousePress(int btn)
{	IR_OnKeyboardPress(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseRelease(int btn)
{	IR_OnKeyboardRelease(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseHold(int btn)
{	IR_OnKeyboardHold(mouse_button_2_key[btn]);}

void CLevel::IR_OnMouseMove( int dx, int dy )
{
	if(g_bDisableAllInput)							return;
	if (CurrentGameUI()->IR_UIOnMouseMove(dx,dy))		return;
	if (Device.Paused() 
#ifdef DEBUG
		&& !psActorFlags.test(AF_NO_CLIP) 
#endif //DEBUG
		)	return;
	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnMouseMove					(dx,dy);
	}
}

// Обработка нажатия клавиш


void CLevel::IR_OnKeyboardPress	(int key)
{
	if(Device.dwPrecacheFrame)
		return;

#ifdef INGAME_EDITOR
	if (Device.editor() && (pInput->iGetAsyncKeyState(DIK_LALT) || pInput->iGetAsyncKeyState(DIK_RALT)))
		return;
#endif // #ifdef INGAME_EDITOR

	bool b_ui_exist = (!!CurrentGameUI());

	EGameActions _curr = get_binded_action(key);

	if(_curr==kPAUSE)
	{
		#ifdef INGAME_EDITOR
			if (Device.editor())	return;
		#endif // INGAME_EDITOR

		return;
	}

	if(	g_bDisableAllInput )	return;

	switch ( _curr ) 
	{
	case kSCREENSHOT:
		Render->Screenshot();
		return;
		break;

	case kCONSOLE:
		pConsole->Show				();
		return;
		break;

	case kQUIT: 
		{
			if(b_ui_exist && CurrentGameUI()->TopInputReceiver() )
			{
					if(CurrentGameUI()->IR_UIOnKeyboardPress(key))	return;//special case for mp and main_menu
					CurrentGameUI()->TopInputReceiver()->HideDialog();
			}else
			{
				pConsoleCommands->Execute("main_menu");
			}return;
		}break;
	};

	if ( !bReady || !b_ui_exist )			return;

	if ( b_ui_exist && CurrentGameUI()->IR_UIOnKeyboardPress(key)) return;

	if ( Device.Paused() 
#ifdef DEBUG
		&& !psActorFlags.test(AF_NO_CLIP) 
#endif //DEBUG
		)	return;

	if ( game && game->OnKeyboardPress(get_binded_action(key)) )	return;

#ifndef MASTER_GOLD
	switch (key) {
	case DIK_DIVIDE: {
		if (!Server)
			break;

		SetGameTimeFactor			(g_fTimeFactor);

#ifdef DEBUG
		if(!m_variables.m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), g_fTimeFactor);
#else //DEBUG
		SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), g_fTimeFactor);
#endif //DEBUG
		
		break;	
	}
	case DIK_MULTIPLY: {
		if (!Server)
			break;

		SetGameTimeFactor			(1000.f);
#ifdef DEBUG
		if(!m_variables.m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), 1000.f);
#else //DEBUG
		SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), 1000.f);
#endif //DEBUG
		
		break;
	}
#ifdef DEBUG
	case DIK_SUBTRACT:{
		if (!Server)
			break;
		if(m_variables.m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), g_fTimeFactor);
		else
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), 0.00001f);

		m_variables.m_bEnvPaused = !m_variables.m_bEnvPaused;
		break;
	}
#endif //DEBUG
	case DIK_NUMPAD5: 
		{
		}
		break;

#ifdef DEBUG

	// Lain: added TEMP!!!
	case DIK_UP:
	{
		break;
	}
	case DIK_DOWN:
	{
		break;
	}
	case DIK_LEFT:
	{
		break;
	}
	case DIK_RIGHT:
	{
		break;
	}

	case DIK_RETURN: {
		bDebug	= !bDebug;
		return;
	}
	case MOUSE_1: {

		break;
	}
	/**/
#endif
#ifdef DEBUG
	case DIK_F9:{
		break;
	}
#endif // DEBUG
	}
#endif // MASTER_GOLD

	if (bindConsoleCmds.execute(key))
		return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnKeyboardPress(get_binded_action(key));
	}


	#ifdef _DEBUG
		CObject *obj = Level().Objects.FindObjectByName("monster");
		if (obj) {
			CBaseMonster *monster = smart_cast<CBaseMonster *>(obj);
			if (monster) 
				monster->debug_on_key(key);
		}
	#endif
}

void CLevel::IR_OnKeyboardRelease(int key)
{
	if (!bReady || g_bDisableAllInput	)								return;
	if ( CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardRelease(key)) return;
	if (game && game->OnKeyboardRelease(get_binded_action(key)) )		return;
	if (Device.Paused() 
#ifdef DEBUG
		&& !psActorFlags.test(AF_NO_CLIP)
#endif //DEBUG
		)				return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnKeyboardRelease			(get_binded_action(key));
	}
}

void CLevel::IR_OnKeyboardHold(int key)
{
	if(g_bDisableAllInput) return;

	if (CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardHold(key)) return;
	if (Device.Paused() 
#ifdef DEBUG
		&& !psActorFlags.test(AF_NO_CLIP)
#endif //DEBUG
		) return;
	if (CURRENT_ENTITY())		{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnKeyboardHold				(get_binded_action(key));
	}
}

void CLevel::IR_OnMouseStop( int /**axis/**/, int /**value/**/)
{
}

void CLevel::IR_OnActivate()
{
	if(!pInput) return;
	int i;
	for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++ )
	{
		if(IR_GetKeyState(i))
		{

			EGameActions action = get_binded_action(i);
			switch (action){
			case kFWD			:
			case kBACK			:
			case kL_STRAFE		:
			case kR_STRAFE		:
			case kLEFT			:
			case kRIGHT			:
			case kUP			:
			case kDOWN			:
			case kCROUCH		:
			case kACCEL			:
			case kL_LOOKOUT		:
			case kR_LOOKOUT		:	
			case kWPN_FIRE		:
				{
					IR_OnKeyboardPress	(i);
				}break;
			};
		};
	}
}
