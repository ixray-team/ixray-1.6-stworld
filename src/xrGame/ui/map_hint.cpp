#include "stdafx.h"
#include "map_hint.h"
#include "UIStatic.h"
#include "UIXmlInit.h"
#include "../map_location.h"
#include "../map_spot.h"
#include "../actor.h"
#include "UIInventoryUtilities.h"
#include "../string_table.h"

CUIStatic* init_static_field(CUIXml& uiXml, LPCSTR path, LPCSTR path2)
{
	CUIStatic* S					= xr_new<CUIStatic>();
	string512						buff;
	S->SetAutoDelete				(true);
	strconcat						(sizeof(buff),buff,path,":",path2);
	CUIXmlInit::InitStatic			(uiXml,buff,0,S);

	return							S;
}

void CUIMapLocationHint::Init(CUIXml& uiXml, LPCSTR path)
{
	CUIXmlInit						xml_init;

	xml_init.InitFrameWindow		(uiXml,path,0,this);

	CUIStatic* S					= NULL;

	S = init_static_field			(uiXml, path, "simple_text");
	AttachChild						(S);
	m_info["simple_text"]			= S;

	S = init_static_field			(uiXml, path, "t_icon");
	AttachChild						(S);
	m_info["t_icon"]				= S;

	S = init_static_field			(uiXml, path, "t_caption");
	AttachChild						(S);
	m_info["t_caption"]				= S;

	S = init_static_field			(uiXml, path, "t_time");
	AttachChild						(S);
	m_info["t_time"]				= S;

	S = init_static_field			(uiXml, path, "t_time_rem");
	AttachChild						(S);
	m_info["t_time_rem"]			= S;

	S = init_static_field			(uiXml, path, "t_hint_text");
	AttachChild						(S);
	m_info["t_hint_text"]			= S;

	m_posx_icon    = m_info["t_icon"]->GetWndPos().x;
	m_posx_caption = m_info["t_caption"]->GetWndPos().x;
}

void CUIMapLocationHint::SetInfoMode(u8 mode)
{
	m_info["simple_text"]->Show		(mode==1);
	m_info["t_icon"]->Show			(mode==2);
	m_info["t_caption"]->Show		(mode==2);
	m_info["t_time"]->Show			(mode==2);
	m_info["t_time_rem"]->Show		(mode==2);
	m_info["t_hint_text"]->Show		(mode==2);
}

void CUIMapLocationHint::Draw_()
{
	inherited::Draw			();
}

void CUIMapLocationHint::SetInfoStr(LPCSTR text)
{
	SetInfoMode				(1);
	CUIStatic* S			= m_info["simple_text"];
	S->TextItemControl()->SetTextST			(text);
	S->AdjustHeightToText	();
	float new_w				= S->GetWndPos().x + S->GetWndSize().x + 20.0f;

	float new_h				= _max(64.0f, S->GetWndPos().y+S->GetWndSize().y+20.0f);
	SetWndSize				(Fvector2().set(new_w, new_h));
}