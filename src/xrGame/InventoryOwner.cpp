#include "stdafx.h"
#include "InventoryOwner.h"
#include "entity_alive.h"
#include "actor.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "level.h"
#include "game_base_space.h"
#include "xrserver.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "game_object_space.h"
#include "CustomOutfit.h"
#include "Bolt.h"

void CInventoryOwner::OnEvent(NET_Packet& P, u16 type)
{
}

CInventoryOwner::CInventoryOwner			()
{
	m_inventory					= xr_new<CInventory>();
	
	bDisableBreakDialog			= false;

	m_tmp_active_slot_num		= NO_ACTIVE_SLOT;

	m_play_show_hide_reload_sounds	= true;
}

DLL_Pure *CInventoryOwner::_construct		()
{
	return						(smart_cast<DLL_Pure*>(this));
}

CInventoryOwner::~CInventoryOwner			() 
{
	xr_delete					(m_inventory);
}

void CInventoryOwner::Load					(LPCSTR section)
{
	if(pSettings->line_exist(section, "inv_max_weight"))
		m_inventory->SetMaxWeight( pSettings->r_float(section,"inv_max_weight") );

}

void CInventoryOwner::reload				(LPCSTR section)
{
	inventory().Clear			();
	inventory().m_pOwner		= this;
	inventory().SetSlotsUseful (true);

	m_money						= 0;

	CAttachmentOwner::reload	(section);
}

void CInventoryOwner::reinit				()
{
	CAttachmentOwner::reinit	();
	m_item_to_spawn				= shared_str();
	m_ammo_in_box_to_spawn		= 0;
}

//call this after CGameObject::net_Spawn
BOOL CInventoryOwner::net_Spawn		(CSE_Abstract* DC)
{
	//получить указатель на объект, InventoryOwner
	//m_inventory->setSlotsBlocked(false);
	CGameObject			*pThis = smart_cast<CGameObject*>(this);
	if(!pThis) return FALSE;
	CSE_Abstract* E	= (CSE_Abstract*)(DC);

	//CharacterInfo().m_SpecificCharacter.Load					("mp_actor");
	//CharacterInfo().InitSpecificCharacter						("mp_actor");
	//CharacterInfo().m_SpecificCharacter.data()->m_sGameName = (E->name_replace()[0]) ? E->name_replace() : *pThis->cName();

	m_game_name												= (E->name_replace()[0]) ? E->name_replace() : *pThis->cName();

	return TRUE;
}

void CInventoryOwner::net_Destroy()
{
	CAttachmentOwner::net_Destroy();
	
	inventory().Clear();
	inventory().SetActiveSlot(NO_ACTIVE_SLOT);
}


void	CInventoryOwner::save	(NET_Packet &output_packet)
{
	if(inventory().GetActiveSlot() == NO_ACTIVE_SLOT)
		output_packet.w_u8((u8)NO_ACTIVE_SLOT);
	else
		output_packet.w_u8((u8)inventory().GetActiveSlot());

	save_data	(m_game_name, output_packet);
	save_data	(m_money,	output_packet);
}
void	CInventoryOwner::load	(IReader &input_packet)
{
	u8 active_slot = input_packet.r_u8();
	if(active_slot == NO_ACTIVE_SLOT)
		inventory().SetActiveSlot(NO_ACTIVE_SLOT);
	//else
		//inventory().Activate_deffered(active_slot, Device.dwFrame);

	m_tmp_active_slot_num		 = active_slot;

	load_data		(m_game_name, input_packet);
	load_data		(m_money,	input_packet);
}


void CInventoryOwner::UpdateInventoryOwner(u32 deltaT)
{
	inventory().Update();

}



void CInventoryOwner::renderable_Render		()
{
	if (inventory().ActiveItem())
		inventory().ActiveItem()->renderable_Render();

	CAttachmentOwner::renderable_Render();
}

void CInventoryOwner::OnItemTake			(CInventoryItem *inventory_item)
{
	CGameObject	*object = smart_cast<CGameObject*>(this);
	VERIFY		(object);

	attach		(inventory_item);

	if(m_tmp_active_slot_num!=NO_ACTIVE_SLOT					&& 
		inventory_item->CurrPlace()==eItemPlaceSlot	&&
		inventory_item->CurrSlot()==m_tmp_active_slot_num)
	{
		if(inventory().ItemFromSlot(m_tmp_active_slot_num))
		{
			inventory().Activate(m_tmp_active_slot_num);
			m_tmp_active_slot_num	= NO_ACTIVE_SLOT;
		}
	}
}

//возвращает текуший разброс стрельбы с учетом движения (в радианах)
float CInventoryOwner::GetWeaponAccuracy	() const
{
	return 0.f;
}

//максимальный переносимы вес
float  CInventoryOwner::MaxCarryWeight () const
{
	float ret =  inventory().GetMaxWeight();

	const CCustomOutfit* outfit	= GetOutfit();
	if(outfit)
		ret += outfit->m_additional_weight2;

	return ret;
}

//игровое имя 
LPCSTR	CInventoryOwner::Name () const
{
//	return CharacterInfo().Name();
	return m_game_name.c_str();
}

LPCSTR	CInventoryOwner::IconName () const
{
	R_ASSERT(0);
	return "mp_actor";//CharacterInfo().IconName().c_str();
}


//////////////////////////////////////////////////////////////////////////
//для работы с relation system
u16 CInventoryOwner::object_id	()  const
{
	return smart_cast<const CGameObject*>(this)->ID();
}

void CInventoryOwner::OnItemDrop(CInventoryItem *inventory_item, bool just_before_destroy)
{
	CGameObject	*object = smart_cast<CGameObject*>(this);
	VERIFY		(object);
	detach		(inventory_item);
}

void CInventoryOwner::OnItemDropUpdate ()
{
}

void CInventoryOwner::OnItemBelt	(CInventoryItem *inventory_item, const SInvItemPlace& previous_place)
{
}

void CInventoryOwner::OnItemRuck	(CInventoryItem *inventory_item, const SInvItemPlace& previous_place)
{
	detach		(inventory_item);
}
void CInventoryOwner::OnItemSlot	(CInventoryItem *inventory_item, const SInvItemPlace& previous_place)
{
	attach		(inventory_item);
}

CCustomOutfit* CInventoryOwner::GetOutfit() const
{
    return smart_cast<CCustomOutfit*>(inventory().ItemFromSlot(OUTFIT_SLOT));
}

void CInventoryOwner::on_weapon_shot_start		(CWeapon *weapon)
{
}

void CInventoryOwner::on_weapon_shot_update		()
{
}

void CInventoryOwner::on_weapon_shot_stop		()
{
}

void CInventoryOwner::on_weapon_shot_remove		(CWeapon *weapon)
{
}

void CInventoryOwner::on_weapon_hide			(CWeapon *weapon)
{
}


void CInventoryOwner::set_money		(u32 amount, bool bSendEvent)
{
	m_money						= amount;

	if(bSendEvent)
	{
		CGameObject				*object = smart_cast<CGameObject*>(this);
		NET_Packet				packet;
		object->u_EventGen		(packet,GE_MONEY,object->ID());
		packet.w_u32			(m_money);
		object->u_EventSend		(packet);
	}
}

bool CInventoryOwner::use_default_throw_force	()
{
	return						(true);
}

float CInventoryOwner::missile_throw_force		() 
{
	NODEFAULT;
#ifdef DEBUG
	return						(0.f);
#endif
}

bool CInventoryOwner::use_throw_randomness		()
{
	return						(true);
}

bool CInventoryOwner::is_alive()
{
	CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(this);
	R_ASSERT( pEntityAlive );
	return (!!pEntityAlive->g_Alive());
}

