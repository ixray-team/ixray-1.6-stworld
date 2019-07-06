#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIDragDropListEx.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "UI3tButton.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIFrameLineWnd.h"

#include "xrMessages.h"
#include "../GameObject.h"
#include "../InventoryOwner.h"
#include "../Inventory.h"
#include "../Inventory_item.h"
#include "../InventoryBox.h"
#include "../string_table.h"

void move_item_from_to (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
	CGameObject::u_EventGen					(P, GE_TRADE_SELL, from_id);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen					(P, GE_TRADE_BUY, to_id);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);
}

bool move_item_check( PIItem itm, CInventoryOwner* from, CInventoryOwner* to, bool weight_check )
{
	if ( weight_check )
	{
		float invWeight		= to->inventory().CalcTotalWeight();
		float maxWeight		= to->MaxCarryWeight();
		float itmWeight		= itm->Weight();
		if ( invWeight + itmWeight >= maxWeight )
		{
			return false;
		}
	}
	move_item_from_to( from->object_id(), to->object_id(), itm->object_id() );
	return true;
}


