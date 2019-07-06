#include "stdafx.h"
#include "UIGameTutorial.h"
#include "UIWindow.h"
#include "UIStatic.h"
#include "UIXmlInit.h"
#include "../../xrEngine/xr_input.h"
#include "../xr_level_controller.h"
#include "../../xrEngine/xr_ioc_cmd.h"
#include "../UIGameCustom.h"
#include "UIActorMenu.h"

extern ENGINE_API BOOL bShowPauseString;


void CUISequenceItem::Load(CUIXml* xml, int idx)
{
	XML_NODE* _stored_root			= xml->GetLocalRoot();
	xml->SetLocalRoot				(xml->NavigateToNode("item",idx));
	int disabled_cnt				= xml->GetNodesNum	(xml->GetLocalRoot(), "disabled_key");
	
	for(int i=0; i<disabled_cnt; ++i)
	{
		LPCSTR str					= xml->Read			("disabled_key", i, NULL);
		m_disabled_actions.push_back( action_name_to_id(str) );
	}

	xml->SetLocalRoot				(_stored_root);
}

bool CUISequenceItem::AllowKey(int dik)
{
	xr_vector<int>::iterator it = std::find(m_disabled_actions.begin(),m_disabled_actions.end(),get_binded_action(dik));
	if(it==m_disabled_actions.end())
		return true;
	else
		return false;
}

void CUISequenceItem::Update()
{
}

void CUISequenceItem::Start()
{
}

bool CUISequenceItem::Stop(bool bForce)
{
	return true;
}

CUISequencer::CUISequencer()
{
	m_flags.zero();
}

void CUISequencer::Start(LPCSTR tutor_name)
{
	VERIFY						(m_sequencer_items.size()==0);
	Device.seqFrame.Add			(this, REG_PRIORITY_LOW-10000);
	
	
	m_UIWindow					= xr_new<CUIWindow>();

	CUIXml uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, "game_tutorials.xml");
	
	int items_count				= uiXml.GetNodesNum	(tutor_name, 0, "item");	VERIFY(items_count>0);
	uiXml.SetLocalRoot			(uiXml.NavigateToNode(tutor_name, 0));

	m_flags.set(etsPlayEachItem, !!uiXml.ReadInt("play_each_item", 0, 0));
	m_flags.set(etsPersistent, !!uiXml.Read("persistent", 0, 0));
	m_flags.set(etsOverMainMenu, !!uiXml.Read("over_main_menu", 0, 0));
	int render_prio				= uiXml.ReadInt("render_prio", 0, -2);

	CUIXmlInit xml_init;
	if(UI().is_widescreen() && uiXml.NavigateToNode("global_wnd_16", 0) )
	{
		xml_init.AssignColor	("tut_gray", color_rgba(255,255,255,255));
		xml_init.InitWindow		(uiXml, "global_wnd_16", 0, m_UIWindow);
	}else
	{
		xml_init.AssignColor	("tut_gray", color_rgba(100,100,100,255));
		xml_init.InitWindow		(uiXml, "global_wnd", 0, m_UIWindow);
	}

	
	XML_NODE* bk				= uiXml.GetLocalRoot();
	uiXml.SetLocalRoot			(uiXml.NavigateToNode("global_wnd", 0));
	{	
		LPCSTR str				= uiXml.Read		("pause_state", 0, "ignore");
		m_flags.set				(etsNeedPauseOn,	0==_stricmp(str, "on"));
		m_flags.set				(etsNeedPauseOff,	0==_stricmp(str, "off"));
	}

	LPCSTR snd_name				= uiXml.Read("sound", 0, "");
	if (snd_name && snd_name[0])
		m_global_sound.create	(snd_name, st_Effect,sg_Undefined);	

	m_start_lua_function		= uiXml.Read("function_on_start", 0, "");
	m_stop_lua_function			= uiXml.Read("function_on_stop", 0, "");

	uiXml.SetLocalRoot			(bk);

	for(int i=0;i<items_count;++i)
	{
		LPCSTR	_tp				= uiXml.ReadAttrib("item",i,"type","");
		bool bVideo				= 0==_stricmp(_tp,"video");
		CUISequenceItem* pItem	= 0;
		if (bVideo)	pItem		= xr_new<CUISequenceVideoItem>(this);
		else		pItem		= xr_new<CUISequenceSimpleItem>(this);
		m_sequencer_items.push_back(pItem);
		pItem->Load				(&uiXml,i);
	}

	Device.seqRender.Add		(this, render_prio /*-2*/);
	
	CUISequenceItem* pCurrItem	= GetNextItem();
	R_ASSERT3					(pCurrItem, "no item(s) to start", tutor_name);
	pCurrItem->Start			();
	m_pStoredInputReceiver		= pInput->CurrentIR();
	IR_Capture					();


	m_flags.set					(etsActive, TRUE);
	m_flags.set					(etsStoredPauseState, Device.Paused());
	
	if(m_flags.test(etsNeedPauseOn) && !m_flags.test(etsStoredPauseState))
	{
		Device.Pause			(TRUE, TRUE, TRUE, "tutorial_start");
		bShowPauseString		= FALSE;
	}

	if(m_flags.test(etsNeedPauseOff) && m_flags.test(etsStoredPauseState))
		Device.Pause			(FALSE, TRUE, FALSE, "tutorial_start");

	if (m_global_sound._handle())		
		m_global_sound.play(NULL, sm_2D);

}

CUISequenceItem* CUISequencer::GetNextItem()
{
	CUISequenceItem* result			= NULL;

	while(m_sequencer_items.size())
	{
		result						= m_sequencer_items.front();
		break;
	}

	return result;
}

extern CUISequencer * g_tutorial;
extern CUISequencer * g_tutorial2;

void CUISequencer::Destroy()
{
	m_global_sound.stop			();
	Device.seqFrame.Remove		(this);
	Device.seqRender.Remove		(this);
	delete_data					(m_sequencer_items);
	delete_data					(m_UIWindow);
	IR_Release					();
	m_flags.set					(etsActive, FALSE);
	m_pStoredInputReceiver		= NULL;
	
	if(!m_on_destroy_event.empty())
		m_on_destroy_event		();

	if(g_tutorial==this)
	{
		g_tutorial = NULL;
	}
	if(g_tutorial2==this)
	{
		g_tutorial2 = NULL;
	}
}

void CUISequencer::Stop()
{
	if(m_sequencer_items.size())
	{ 
		if (m_flags.test(etsPlayEachItem))
		{
			Next				();
			return;
		}else
		{
			CUISequenceItem* pCurrItem	= m_sequencer_items.front();
			pCurrItem->Stop				(true);
		}
	}
	{
	if(m_flags.test(etsNeedPauseOn) && !m_flags.test(etsStoredPauseState))
		Device.Pause			(FALSE, TRUE, TRUE, "tutorial_stop");

	if(m_flags.test(etsNeedPauseOff) && m_flags.test(etsStoredPauseState))
		Device.Pause			(TRUE, TRUE, FALSE, "tutorial_stop");
	}
	Destroy			();
}

void CUISequencer::OnFrame()
{  
	if(!Device.b_is_Active)		return;
	if(!IsActive() )			return;

	if(!m_sequencer_items.size())
	{
		Stop					();
		return;
	}else
	{
		CUISequenceItem* pCurrItem	= m_sequencer_items.front();
		if(!pCurrItem->IsPlaying())	
			Next();
	}

	if(!m_sequencer_items.size())
	{
		Stop					();
		return;
	}

	VERIFY(m_sequencer_items.front());
	m_sequencer_items.front()->Update		();
	m_UIWindow->Update			();
}

void CUISequencer::OnRender	()
{
	if (m_UIWindow->IsShown())	
		m_UIWindow->Draw();

	VERIFY(m_sequencer_items.size());
	m_sequencer_items.front()->OnRender	();
}

void CUISequencer::Next		()
{
	CUISequenceItem* pCurrItem	= m_sequencer_items.front();
	bool can_stop				= pCurrItem->Stop();
	if	(!can_stop)				
		return;

	m_sequencer_items.pop_front	();
	delete_data					(pCurrItem);

	if(m_sequencer_items.size())
	{
		pCurrItem				= GetNextItem();
		if(pCurrItem)
			pCurrItem->Start	();
	}
}

bool CUISequencer::GrabInput()
{
	if(m_sequencer_items.size())
		return m_sequencer_items.front()->GrabInput();
	else
	return false;

}

void CUISequencer::IR_OnMousePress		(int btn)
{
	if(m_sequencer_items.size())	
		m_sequencer_items.front()->OnMousePress	(btn);

	if(!GrabInput() && m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMousePress(btn);
}

void CUISequencer::IR_OnMouseRelease		(int btn)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMouseRelease(btn);
}

void CUISequencer::IR_OnMouseHold		(int btn)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMouseHold(btn);
}

void CUISequencer::IR_OnMouseMove		(int x, int y)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMouseMove(x, y);
}

void CUISequencer::IR_OnMouseStop		(int x, int y)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMouseStop(x, y);
}

void CUISequencer::IR_OnKeyboardRelease	(int dik)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnKeyboardRelease(dik);
}


void CUISequencer::IR_OnKeyboardHold		(int dik)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnKeyboardHold(dik);
}


void CUISequencer::IR_OnMouseWheel		(int direction)
{
	if(!GrabInput()&&m_pStoredInputReceiver)
		m_pStoredInputReceiver->IR_OnMouseWheel(direction);
}

void CUISequencer::IR_OnKeyboardPress	(int dik)
{
	if(m_sequencer_items.size())	
		m_sequencer_items.front()->OnKeyboardPress			(dik);
	
	bool b = true;
	if(m_sequencer_items.size()) b &= m_sequencer_items.front()->AllowKey(dik);

	bool binded = is_binded(kQUIT, dik);
	if(b && binded )
	{
		Stop		();
		return;
	}

	if(binded && CurrentGameUI())
	{
		if(CurrentGameUI()->ActorMenu().IsShown())
		{
			CurrentGameUI()->HideActorMenu();
			return;
		}
		pConsoleCommands->Execute("main_menu");
		return;
	}

	if(b && !GrabInput() && m_pStoredInputReceiver)	
		m_pStoredInputReceiver->IR_OnKeyboardPress	(dik);
}

void CUISequencer::IR_OnActivate()
{
	if(!pInput) return;
	int i;
	for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++ )
	{
		if(IR_GetKeyState(i))
		{
			EGameActions action		= get_binded_action(i);
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
