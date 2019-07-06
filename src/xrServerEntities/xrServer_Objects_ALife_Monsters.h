////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects monsters for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_MonstersH
#define xrServer_Objects_ALife_MonstersH

#include "xrServer_Objects_ALife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "associative_vector.h"

#pragma warning(push)
#pragma warning(disable:4005)

SERVER_ENTITY_DECLARE_BEGIN0(CSE_ALifeTraderAbstract)
	enum eTraderFlags {
		eTraderFlagInfiniteAmmo		= u32(1) << 0,
		eTraderFlagDummy			= u32(-1),
	};
//	float							m_fCumulativeItemMass;
//	int								m_iCumulativeItemVolume;
	u32								m_dwMoney;
	float							m_fMaxItemMass;
	Flags32							m_trader_flags;

	xr_string						m_character_name;
	
#ifdef XRGAME_EXPORTS
	//для работы с relation system
	u16								object_id				() const;
#endif

public:	
									CSE_ALifeTraderAbstract		(LPCSTR caSection);
	virtual							~CSE_ALifeTraderAbstract	();
	// we need this to prevent virtual inheritance :-(
	virtual CSE_Abstract			*base						() = 0;
	virtual const CSE_Abstract		*base						() const = 0;
	virtual CSE_Abstract			*init						();
	virtual CSE_Abstract			*cast_abstract				() {return 0;};
	virtual CSE_ALifeTraderAbstract	*cast_trader_abstract		() {return this;};
	// end of the virtual inheritance dependant code
			void __stdcall			OnChangeProfile				(PropValue* sender);

#ifdef XRGAME_EXPORTS

#if 0//def DEBUG
			bool					check_inventory_consistency	();
#endif
			void					vfInitInventory				();
#endif
SERVER_ENTITY_DECLARE_END


SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCustomZone,CSE_ALifeSpaceRestrictor)
//.	f32								m_maxPower;
	ALife::EHitType					m_tHitType;
	u32								m_owner_id;
	u32								m_enabled_time;
	u32								m_disabled_time;
	u32								m_start_time_shift;

									CSE_ALifeCustomZone		(LPCSTR caSection);
	virtual							~CSE_ALifeCustomZone	();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeAnomalousZone,CSE_ALifeCustomZone)
	CSE_ALifeItemWeapon				*m_tpCurrentBestWeapon;
	float							m_offline_interactive_radius;
	u32								m_artefact_position_offset;
	u16								m_artefact_spawn_count;

									CSE_ALifeAnomalousZone	(LPCSTR caSection);
	virtual							~CSE_ALifeAnomalousZone	();
	virtual CSE_Abstract			*init					();
	virtual CSE_Abstract			*base					();
	virtual const CSE_Abstract		*base					() const;
	virtual CSE_Abstract			*cast_abstract			() {return this;};
	virtual CSE_ALifeAnomalousZone	*cast_anomalous_zone	() {return this;};
#ifdef XRGAME_EXPORTS
//			void					spawn_artefacts			();
	virtual void					on_spawn				();
	virtual bool					keep_saved_data_anyway	() const;
#endif
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeTorridZone,CSE_ALifeCustomZone,CSE_Motion)
									CSE_ALifeTorridZone		(LPCSTR caSection);
	virtual							~CSE_ALifeTorridZone	();
	virtual CSE_Motion*	__stdcall	motion					();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeZoneVisual,CSE_ALifeAnomalousZone,CSE_Visual)
shared_str attack_animation;
CSE_ALifeZoneVisual	(LPCSTR caSection);
virtual							~CSE_ALifeZoneVisual	();
virtual CSE_Visual* __stdcall	visual					();
SERVER_ENTITY_DECLARE_END

//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCreatureAbstract,CSE_ALifeDynamicObjectVisual)
private:
	float							fHealth;
	ALife::_OBJECT_ID				m_killer_id;
public:
	u8								s_team;
	u8								s_squad;
	u8								s_group;
	
	float							m_fMorale;
	float							m_fAccuracy;
	float							m_fIntelligence;

	u32								timestamp;				// server(game) timestamp
	u8								flags;
	float							o_model;				// model yaw
	SRotation						o_torso;				// torso in world coords
	bool							m_bDeathIsProcessed;

	xr_vector<ALife::_OBJECT_ID>	m_dynamic_out_restrictions;
	xr_vector<ALife::_OBJECT_ID>	m_dynamic_in_restrictions;

	ALife::_TIME_ID					m_game_death_time;
									
									CSE_ALifeCreatureAbstract(LPCSTR caSection);
	virtual							~CSE_ALifeCreatureAbstract();
	virtual u8						g_team					();
	virtual u8						g_squad					();
	virtual u8						g_group					();
	
	IC		float					get_health				() const								{ return fHealth;}
	IC		ALife::_OBJECT_ID		get_killer_id			() const								{ return m_killer_id; }

			void					set_health				(float const health_value);
			void					set_killer_id			(ALife::_OBJECT_ID const killer_id);

	IC		bool					g_Alive					() const								{ return (get_health() > 0.f);}
//	virtual bool					used_ai_locations		() const;
	virtual CSE_ALifeCreatureAbstract	*cast_creature_abstract		() {return this;};
#ifdef XRGAME_EXPORTS
	virtual	void					on_death				(CSE_Abstract *killer){};
	virtual void					on_spawn				();
#endif
#ifdef DEBUG
	virtual bool					match_configuration		() const;
#endif
SERVER_ENTITY_DECLARE_END


SERVER_ENTITY_DECLARE_BEGIN3(CSE_ALifeCreatureActor,CSE_ALifeCreatureAbstract,CSE_ALifeTraderAbstract,CSE_PHSkeleton)
	
	u16								mstate;
	Fvector							accel;
	Fvector							velocity;
//	float							fArmor;
	float							fRadiation;
	u8								weapon;
	///////////////////////////////////////////
	u16								m_u16NumItems;
	u16								m_holderID;
//	DEF_DEQUE		(PH_STATES, SPHNetState); 
	SPHNetState						m_AliveState;
//	PH_STATES						m_DeadStates;

	// статический массив - 6 float(вектора пределов квантизации) + m_u16NumItems*(7 u8) (позиция и поворот кости)
	u8								m_BoneDataSize;
	char							m_DeadBodyData[1024];
	///////////////////////////////////////////
									CSE_ALifeCreatureActor	(LPCSTR caSection);
	virtual							~CSE_ALifeCreatureActor	();
	virtual CSE_Abstract			*base					();
	virtual const CSE_Abstract		*base					() const;
	virtual CSE_Abstract			*init					();
	virtual bool					can_save				()const{return true;}

#ifdef DEBUG
	virtual bool					match_configuration		() const;
#endif
	virtual CSE_Abstract			*cast_abstract			() {return this;};
	virtual CSE_ALifeTraderAbstract	*cast_trader_abstract	() {return this;};
public:
	virtual BOOL					Net_Relevant			();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCreatureCrow,CSE_ALifeCreatureAbstract)
									CSE_ALifeCreatureCrow	(LPCSTR caSection);
	virtual							~CSE_ALifeCreatureCrow	();
//	virtual bool					used_ai_locations		() const;
SERVER_ENTITY_DECLARE_END

#pragma warning(pop)

#endif