#include "stdafx.h"
#include "UIActorMenu.h"
#include "UI3tButton.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICharacterInfo.h"
#include "UIFrameLineWnd.h"
#include "UICellItem.h"
#include "UIInventoryUtilities.h"
#include "UICellItemFactory.h"

#include "../InventoryOwner.h"
#include "../Inventory.h"
#include "../Entity.h"
#include "../Actor.h"
#include "../Weapon.h"
#include "../inventory_item_object.h"
#include "../string_table.h"

// -------------------------------------------------


bool is_item_in_list(CUIDragDropListEx* pList, PIItem item)
{
	for(u16 i=0;i<pList->ItemsCount();i++)
	{
		CUICellItem* cell_item = pList->GetItemIdx(i);
		for(u16 k=0;k<cell_item->ChildsCount();k++)
		{
			CUICellItem* inv_cell_item = cell_item->Child(k);
			if((PIItem)inv_cell_item->m_pData==item)
				return true;
		}
		if((PIItem)cell_item->m_pData==item)
			return true;
	}
	return false;
}



void CUIActorMenu::ColorizeItem(CUICellItem* itm, bool colorize)
{
	if( colorize )
	{
		itm->SetTextureColor( color_rgba(255,100,100,255) );
	}else
	{
		itm->SetTextureColor( color_rgba(255,255,255,255) );
	}
}

float CUIActorMenu::CalcItemsWeight(CUIDragDropListEx* pList)
{
	float res = 0.0f;

	for( u32 i = 0; i < pList->ItemsCount(); ++i )
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		PIItem	iitem		= (PIItem)itm->m_pData;
		res					+= iitem->Weight();
		for( u32 j = 0; j < itm->ChildsCount(); ++j )
		{
			PIItem	jitem	= (PIItem)itm->Child(j)->m_pData;
			res				+= jitem->Weight();
		}
	}
	return res;
}


void CUIActorMenu::UpdateActor()
{
	UpdateActorMP			();
	
	CActor* actor = smart_cast<CActor*>( m_pActorInvOwner );
	if ( actor )
	{
		CWeapon* wp = smart_cast<CWeapon*>( actor->inventory().ActiveItem() );
		if ( wp ) 
		{
			wp->ForceUpdateAmmo();
		}
	}//actor

	InventoryUtilities::UpdateWeightStr( *m_ActorWeight, *m_ActorWeightMax, m_pActorInvOwner );
	
	m_ActorWeight->AdjustWidthToText();
	m_ActorWeightMax->AdjustWidthToText();
	m_ActorBottomInfo->AdjustWidthToText();

	Fvector2 pos = m_ActorWeight->GetWndPos();
	pos.x = m_ActorWeightMax->GetWndPos().x - m_ActorWeight->GetWndSize().x - 5.0f;
	m_ActorWeight->SetWndPos( pos );
	pos.x = pos.x - m_ActorBottomInfo->GetWndSize().x - 5.0f;
	m_ActorBottomInfo->SetWndPos( pos );
}


void CUIActorMenu::UpdatePrices()
{
//	LPCSTR kg_str = CStringTable().translate( "st_kg" ).c_str();
//
//	UpdateActor();
//	UpdatePartnerBag();
//	u32 actor_price   = CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
//	u32 partner_price = CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );
//
//	string64 buf;
//	xr_sprintf( buf, "%d RU", actor_price );		m_ActorTradePrice->SetText( buf );	m_ActorTradePrice->AdjustWidthToText();
//	xr_sprintf( buf, "%d RU", partner_price );	m_PartnerTradePrice->SetText( buf );	m_PartnerTradePrice->AdjustWidthToText();
//
//	float actor_weight   = CalcItemsWeight( m_pTradeActorList );
//	float partner_weight = CalcItemsWeight( m_pTradePartnerList );
//
//	xr_sprintf( buf, "(%.1f %s)", actor_weight, kg_str );	m_ActorTradeWeightMax->SetText( buf );
//	xr_sprintf( buf, "(%.1f %s)", partner_weight, kg_str );	m_PartnerTradeWeightMax->SetText( buf );
//
//	Fvector2 pos = m_ActorTradePrice->GetWndPos();
//	pos.x = m_ActorTradeWeightMax->GetWndPos().x - m_ActorTradePrice->GetWndSize().x - 5.0f;
//	m_ActorTradePrice->SetWndPos( pos );
////	pos.x = pos.x - m_ActorTradeCaption->GetWndSize().x - 5.0f;
////	m_ActorTradeCaption->SetWndPos( pos );
//
//	pos = m_PartnerTradePrice->GetWndPos();
//	pos.x = m_PartnerTradeWeightMax->GetWndPos().x - m_PartnerTradePrice->GetWndSize().x - 5.0f;
//	m_PartnerTradePrice->SetWndPos( pos );
////	pos.x = pos.x - m_PartnerTradeCaption->GetWndSize().x - 5.0f;
////	m_PartnerTradeCaption->SetWndPos( pos );
}

void CUIActorMenu::OnBtnPerformTradeBuy(CUIWindow* w, void* d)
{
//	if(m_pTradePartnerList->ItemsCount()==0) 
//	{
//		return;
//	}
//
//	int actor_money    = (int)m_pActorInvOwner->get_money();
//	int partner_money  = (int)m_pPartnerInvOwner->get_money();
//	int actor_price    = 0;//(int)CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
//	int partner_price  = (int)CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );
//
//	int delta_price    = actor_price - partner_price;
//	actor_money        += delta_price;
//	partner_money      -= delta_price;
//
//	if ( ( actor_money >= 0 ) /*&& ( partner_money >= 0 )*/ && ( actor_price >= 0 || partner_price > 0 ) )
//	{
//		m_partner_trade->OnPerformTrade( partner_price, actor_price );
//
////		TransferItems( m_pTradeActorList,   m_pTradePartnerBagList, m_partner_trade, true );
//		TransferItems( m_pTradePartnerList,	m_pTradeActorBagList,	m_partner_trade, false );
//	}
//	else
//	{
//		if ( actor_money < 0 )
//		{
//			CallMessageBoxOK( "not_enough_money_actor" );
//		}
//		//else if ( partner_money < 0 )
//		//{
//		//	CallMessageBoxOK( "not_enough_money_partner" );
//		//}
//		else
//		{
//			CallMessageBoxOK( "trade_dont_make" );
//		}
//	}
//	SetCurrentItem					( NULL );
//
//	UpdateItemsPlace				();
}
void CUIActorMenu::OnBtnPerformTradeSell(CUIWindow* w, void* d)
{
//	if ( m_pTradeActorList->ItemsCount() == 0 ) 
//	{
//		return;
//	}
//
//	int actor_money    = (int)m_pActorInvOwner->get_money();
//	int partner_money  = (int)m_pPartnerInvOwner->get_money();
//	int actor_price    = (int)CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
//	int partner_price  = 0;//(int)CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );
//
//	int delta_price    = actor_price - partner_price;
//	actor_money        += delta_price;
//	partner_money      -= delta_price;
//
//	if ( ( actor_money >= 0 ) && ( partner_money >= 0 ) && ( actor_price >= 0 || partner_price > 0 ) )
//	{
//		m_partner_trade->OnPerformTrade( partner_price, actor_price );
//
//		TransferItems( m_pTradeActorList,   m_pTradePartnerBagList, m_partner_trade, true );
////		TransferItems( m_pTradePartnerList,	m_pTradeActorBagList,	m_partner_trade, false );
//	}
//	else
//	{
///*		if ( actor_money < 0 )
//		{
//			CallMessageBoxOK( "not_enough_money_actor" );
//		}
//		else */if ( partner_money < 0 )
//		{
//			CallMessageBoxOK( "not_enough_money_partner" );
//		}
//		else
//		{
//			CallMessageBoxOK( "trade_dont_make" );
//		}
//	}
//	SetCurrentItem					( NULL );
//
//	UpdateItemsPlace				();
}