#include "stdafx.h"

#include "UIInventoryUtilities.h"

#include "uicharacterinfo.h"
#include "../actor.h"
#include "../level.h"
#include "../string_table.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "uistatic.h"
#include "UIScrollView.h"

#include "../xrServer.h"
#include "../../xrServerEntities/xrServer_Objects_ALife_Monsters.h"

using namespace InventoryUtilities;

CSE_ALifeTraderAbstract* ch_info_get_from_id (u16 id)
{
	return	smart_cast<CSE_ALifeTraderAbstract*>(Level().Server->game->get_entity_from_eid(id));
}

CUICharacterInfo::CUICharacterInfo()
	: m_ownerID(u16(-1)),
	pUIBio(NULL)
{
	ZeroMemory			(m_icons,sizeof(m_icons));
	m_bForceUpdate		= false;
	m_texture_name		= NULL;
}

CUICharacterInfo::~CUICharacterInfo()
{}

void CUICharacterInfo::InitCharacterInfo(Fvector2 pos, Fvector2 size, CUIXml* xml_doc)
{
	inherited::SetWndPos(pos);
	inherited::SetWndSize(size);

	Init_IconInfoItem( *xml_doc, "icon",                eIcon         );
	Init_IconInfoItem( *xml_doc, "icon_over",           eIconOver     );

/*	Init_IconInfoItem( *xml_doc, "rank_icon",           eRankIcon     );
	Init_IconInfoItem( *xml_doc, "rank_icon_over",      eRankIconOver );

	Init_IconInfoItem( *xml_doc, "commumity_icon",      eCommunityIcon     );
	Init_IconInfoItem( *xml_doc, "commumity_icon_over", eCommunityIconOver );

	Init_IconInfoItem( *xml_doc, "commumity_big_icon",      eCommunityBigIcon     );
	Init_IconInfoItem( *xml_doc, "commumity_big_icon_over", eCommunityBigIconOver );
*/
	VERIFY( m_icons[eIcon] );
	m_deadbody_color = color_argb(160,160,160,160);
	if ( xml_doc->NavigateToNode( "icon:deadbody", 0 ) )
	{
		m_deadbody_color = CUIXmlInit::GetColor( *xml_doc, "icon:deadbody", 0, m_deadbody_color );
	}

	// ----------------------------
	Init_StrInfoItem( *xml_doc, "name_caption",      eNameCaption       );
	Init_StrInfoItem( *xml_doc, "name_static",       eName              );

	Init_StrInfoItem( *xml_doc, "rank_caption",      eRankCaption       );
	Init_StrInfoItem( *xml_doc, "rank_static",       eRank              );
	
	Init_StrInfoItem( *xml_doc, "community_caption", eCommunityCaption  );
	Init_StrInfoItem( *xml_doc, "community_static",  eCommunity         );

	Init_StrInfoItem( *xml_doc, "reputation_caption",eReputationCaption );
	Init_StrInfoItem( *xml_doc, "reputation_static", eReputation        );

	Init_StrInfoItem( *xml_doc, "relation_caption",  eRelationCaption   );
	Init_StrInfoItem( *xml_doc, "relation_static",   eRelation          );

	if (xml_doc->NavigateToNode("biography_list", 0))
	{
		pUIBio = xr_new<CUIScrollView>();
		pUIBio->SetAutoDelete(true);
		CUIXmlInit::InitScrollView( *xml_doc, "biography_list", 0, pUIBio );
		AttachChild(pUIBio);
	}
}

void CUICharacterInfo::Init_StrInfoItem( CUIXml& xml_doc, LPCSTR item_str, UIItemType type )
{
	if ( xml_doc.NavigateToNode( item_str, 0 ) )
	{
		CUIStatic*	pItem = m_icons[type] = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic( xml_doc, item_str, 0, pItem );
		AttachChild( pItem );
		pItem->SetAutoDelete( true );
	}
}

void CUICharacterInfo::Init_IconInfoItem( CUIXml& xml_doc, LPCSTR item_str, UIItemType type )
{
	if ( xml_doc.NavigateToNode( item_str, 0 ) )
	{
		CUIStatic*	pItem = m_icons[type] = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic( xml_doc, item_str, 0, pItem );
		
//.		pItem->ClipperOn();
		pItem->Show( true );
		pItem->Enable( true );
		AttachChild( pItem );
		pItem->SetAutoDelete( true );
	}
}

void CUICharacterInfo::InitCharacterInfo(Fvector2 pos, Fvector2 size, LPCSTR xml_name)
{
	CUIXml						uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, xml_name);
	InitCharacterInfo			(pos, size,&uiXml);
}

void CUICharacterInfo::InitCharacterInfo(CUIXml* xml_doc, LPCSTR node_str)
{
	Fvector2 pos, size;
	XML_NODE* stored_root		= xml_doc->GetLocalRoot();
	XML_NODE* ch_node			= xml_doc->NavigateToNode(node_str,0);
	xml_doc->SetLocalRoot		(ch_node);
	pos.x						= xml_doc->ReadAttribFlt(ch_node, "x");
	pos.y						= xml_doc->ReadAttribFlt(ch_node, "y");
	size.x						= xml_doc->ReadAttribFlt(ch_node, "width");
	size.y						= xml_doc->ReadAttribFlt(ch_node, "height");
	InitCharacterInfo			(pos, size, xml_doc);
	xml_doc->SetLocalRoot		(stored_root);
}



void CUICharacterInfo::InitCharacterMP( LPCSTR player_name, LPCSTR player_icon )
{
	ClearInfo();
	
	if ( m_icons[eName] )
	{
		m_icons[eName]->TextItemControl()->SetTextST( player_name );
		m_icons[eName]->Show( true );
	}

	if ( m_icons[eIcon] )
	{
		m_icons[eIcon]->InitTexture( player_icon );
		m_icons[eIcon]->Show( true );
	}
	if ( m_icons[eIconOver] )
	{
		m_icons[eIconOver]->Show( true );
	}
}



//////////////////////////////////////////////////////////////////////////

void CUICharacterInfo::ResetAllStrings()
{
	if(m_icons[eName])			m_icons[eName]->TextItemControl()->SetText			("");
	if(m_icons[eRank])			m_icons[eRank]->TextItemControl()->SetText			("");
	if(m_icons[eCommunity])		m_icons[eCommunity]->TextItemControl()->SetText		("");
	if(m_icons[eReputation])	m_icons[eReputation]->TextItemControl()->SetText	("");
	if(m_icons[eRelation])		m_icons[eRelation]->TextItemControl()->SetText		("");
}


void CUICharacterInfo::Update()
{
	inherited::Update();

	if ( hasOwner() && ( m_bForceUpdate ||(Device.dwFrame%50 == 0) )  )
	{
		m_bForceUpdate = false;

		m_ownerID = u16(-1);

	}
}

void CUICharacterInfo::ClearInfo()
{
	ResetAllStrings	();
	
	for ( int i = eIcon; i < eMaxCaption; ++i )
	{
		if ( m_icons[i] )
		{
			m_icons[i]->Show( false );
		}
	}
}

