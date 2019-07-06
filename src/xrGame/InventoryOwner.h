/////////////////////////////////////////////////////
// Для персонажей, имеющих инвентарь
// InventoryOwner.h
//////////////////////////////////////////////////////

#pragma once

#include "attachment_owner.h"
#include "inventory_space.h"

class CSE_Abstract;
class CInventory;
class CInventoryItem;
class CGameObject;
class CEntityAlive;
class CCustomZone;
class CInfoPortionWrapper;
class NET_Packet;
class CSpecificCharacter;
class CTradeParameters;
class CPurchaseList;
class CWeapon;
class CCustomOutfit;

class CInventoryOwner : public CAttachmentOwner {							
public:
					CInventoryOwner				();
	virtual			~CInventoryOwner			();

public:
	virtual CInventoryOwner*	cast_inventory_owner	()						{return this;}
public:

	virtual DLL_Pure	*_construct				();
	virtual BOOL		net_Spawn				(CSE_Abstract* DC);
	virtual void		net_Destroy				();
			void		Init					();
	virtual void		Load					(LPCSTR section);
	virtual void		reinit					();
	virtual void		reload					(LPCSTR section);
	virtual void		OnEvent					(NET_Packet& P, u16 type);

	//serialization
	virtual void	save						(NET_Packet &output_packet);
	virtual void	load						(IReader &input_packet);

	
	//обновление
	virtual void	UpdateInventoryOwner		(u32 deltaT);
	virtual bool	CanPutInSlot				(PIItem item, u32 slot){return true;};


	// инвентарь
	CInventory	*m_inventory;			
	
	////////////////////////////////////
	//торговля и общение с персонажем

	virtual void	OnFollowerCmd		(int cmd)		{};//redefine for CAI_Stalkker
			bool	bDisableBreakDialog;

	//игровое имя 
	virtual LPCSTR	Name        () const;
	LPCSTR				IconName		() const;
	u32					get_money		() const				{return m_money;}
	void				set_money		(u32 amount, bool bSendEvent);
	bool				is_alive		();

protected:
	u32					m_money;

	u16					m_tmp_active_slot_num;
	
	bool				m_play_show_hide_reload_sounds;

	//////////////////////////////////////////////////////////////////////////
	// инвентарь 
public:
	const CInventory &inventory() const {VERIFY (m_inventory); return(*m_inventory);}
	CInventory		 &inventory()		{VERIFY (m_inventory); return(*m_inventory);}

	//возвращает текуший разброс стрельбы (в радианах) с учетом движения
	virtual float GetWeaponAccuracy			() const;
	//максимальный переносимы вес
	virtual float MaxCarryWeight			() const;

	CCustomOutfit* GetOutfit				() const;

	bool CanPlayShHdRldSounds				() const {return m_play_show_hide_reload_sounds;};
	void SetPlayShHdRldSounds				(bool play) {m_play_show_hide_reload_sounds = play;};
//////////////////////////////////////////////////////////////////////////
	//игровые характеристики персонажа
public:

	//для работы с relation system
	u16								object_id	() const;
protected:
	xr_string				m_game_name;

public:
	virtual void			renderable_Render		();
	virtual void			OnItemTake				(CInventoryItem *inventory_item);
	
	virtual void			OnItemBelt				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	virtual void			OnItemRuck				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	virtual void			OnItemSlot				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	
	virtual void			OnItemDrop				(CInventoryItem *inventory_item, bool just_before_destroy);
	virtual void			OnItemDropUpdate		();
	virtual bool			use_bolts				() const {return(true);}

protected:
	shared_str					m_item_to_spawn;
	u32							m_ammo_in_box_to_spawn;

public:
	IC		const shared_str	&item_to_spawn			() const {return m_item_to_spawn;}
	IC		const u32			&ammo_in_box_to_spawn	() const {return m_ammo_in_box_to_spawn;}

public:
	virtual bool				unlimited_ammo			()	= 0;
	virtual	void				on_weapon_shot_start	(CWeapon *weapon);
	virtual	void				on_weapon_shot_update	();
	virtual	void				on_weapon_shot_stop		();
	virtual	void				on_weapon_shot_remove	(CWeapon *weapon);
	virtual	void				on_weapon_hide			(CWeapon *weapon);

public:
	virtual	bool				use_simplified_visual	() const {return (false);};

public:
			void				buy_supplies			(CInifile &ini_file, LPCSTR section);
	virtual bool				can_use_dynamic_lights	() {return true;}
	virtual	bool				use_default_throw_force	();
	virtual	float				missile_throw_force		(); 
	virtual	bool				use_throw_randomness	();

};

#include "inventory_owner_inline.h"