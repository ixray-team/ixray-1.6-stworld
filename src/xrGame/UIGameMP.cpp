#include "stdafx.h"
#include "UIGameMP.h"
#include "UIAchivementsIndicator.h"
#include "UICursor.h"
#include "Level.h"
#include "game_cl_mp.h"

UIGameMP::UIGameMP() 
:m_pAchivementIdicator(NULL),
m_game(NULL)
{}

UIGameMP::~UIGameMP()
{}

bool UIGameMP::IR_UIOnKeyboardPress(int dik)
{
	return inherited::IR_UIOnKeyboardPress(dik);
}

bool UIGameMP::IR_UIOnKeyboardRelease(int dik)
{
	return inherited::IR_UIOnKeyboardRelease(dik);
}

//bool UIGameMP::ShowServerInfo()
//{
//	if (Level().IsDemoPlay())
//		return true;
//
//	VERIFY2(m_pServerInfo, "game client UI not created");
//	if (!m_pServerInfo)
//	{
//		return false;
//	}
//
//	if (!m_pServerInfo->HasInfo())
//	{
//		m_game->OnMapInfoAccept();
//		return true;
//	}
//
//	if (!m_pServerInfo->IsShown())
//	{
//		m_pServerInfo->ShowDialog(true);
//	}
//	return true;
//}

void UIGameMP::SetClGame(game_cl_GameState* g)
{
	inherited::SetClGame(g);
	m_game = smart_cast<game_cl_mp*>(g);
	VERIFY(m_game);

	m_pAchivementIdicator	= xr_new<CUIAchivementIndicator>();
	m_pAchivementIdicator->SetAutoDelete(true);
	m_window->AttachChild	(m_pAchivementIdicator);
}

void UIGameMP::AddAchivment(shared_str const & achivement_name,
							shared_str const & color_animation,
							u32 const width,
							u32 const height)
{
	VERIFY(m_pAchivementIdicator);
	m_pAchivementIdicator->AddAchivement(achivement_name, color_animation, width, height);
}



