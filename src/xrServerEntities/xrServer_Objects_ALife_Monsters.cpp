////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects monsters for ALife simulator
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_Objects_ALife_Monsters.h"



#ifdef XRGAME_EXPORTS
#	include "string_table.h"
#	include "date_time.h"
#endif

void setup_location_types_section(GameGraph::TERRAIN_VECTOR &m_vertex_types, CInifile const * ini, LPCSTR section)
{
	VERIFY3							(ini->section_exist(section),"cannot open section",section);
	GameGraph::STerrainPlace		terrain_mask;
	terrain_mask.tMask.resize		(GameGraph::LOCATION_TYPE_COUNT);

	CInifile::Sect& sect			= ini->r_section(section);
	CInifile::SectCIt				I = sect.Data.begin();
	CInifile::SectCIt				E = sect.Data.end();
	for ( ; I != E; ++I) {
		LPCSTR						S = *(*I).first;
		string16					I;
		u32							N = _GetItemCount(S);
		
		if (N != GameGraph::LOCATION_TYPE_COUNT)
			continue;

		for (u32 j=0; j<GameGraph::LOCATION_TYPE_COUNT; ++j)
			terrain_mask.tMask[j]	= GameGraph::_LOCATION_ID(atoi(_GetItem(S,j,I)));
		
		m_vertex_types.push_back	(terrain_mask);
	}
	
	if (!m_vertex_types.empty())
		return;

	for (u32 j=0; j<GameGraph::LOCATION_TYPE_COUNT; ++j)
		terrain_mask.tMask[j]		= 255;
	
	m_vertex_types.push_back		(terrain_mask);
}

void setup_location_types_line(GameGraph::TERRAIN_VECTOR &m_vertex_types, LPCSTR string)
{
	string16						I;
	GameGraph::STerrainPlace		terrain_mask;
	terrain_mask.tMask.resize		(GameGraph::LOCATION_TYPE_COUNT);
	
	u32								N = _GetItemCount(string)/GameGraph::LOCATION_TYPE_COUNT*GameGraph::LOCATION_TYPE_COUNT;
	
	if (!N) {
		for (u32 j=0; j<GameGraph::LOCATION_TYPE_COUNT; ++j)
			terrain_mask.tMask[j]	= 255;
		m_vertex_types.push_back	(terrain_mask);
		return;
	}

	m_vertex_types.reserve			(32);

	for (u32 i=0; i<N;) {
		for (u32 j=0; j<GameGraph::LOCATION_TYPE_COUNT; ++j, ++i)
			terrain_mask.tMask[j]	= GameGraph::_LOCATION_ID(atoi(_GetItem(string,i,I)));
		m_vertex_types.push_back	(terrain_mask);
	}
}

void setup_location_types(GameGraph::TERRAIN_VECTOR &m_vertex_types, CInifile const * ini, LPCSTR string)
{
	m_vertex_types.clear			();
	if (ini->section_exist(string) && ini->line_count(string))
		setup_location_types_section(m_vertex_types,ini,string);
	else 
		setup_location_types_line	(m_vertex_types,string);
}

//////////////////////////////////////////////////////////////////////////

//возможное отклонение от значения репутации
//заданого в профиле и для конкретного персонажа
#define REPUTATION_DELTA	10
#define RANK_DELTA			10


//////////////////////////////////////////////////////////////////////////

using namespace ALife;

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeTraderAbstract
////////////////////////////////////////////////////////////////////////////
CSE_ALifeTraderAbstract::CSE_ALifeTraderAbstract(LPCSTR caSection)
{
	m_dwMoney					= 0;
	if (pSettings->line_exist(caSection, "money"))
		m_dwMoney 				= pSettings->r_u32(caSection, "money");
	m_fMaxItemMass				= pSettings->r_float(caSection, "max_item_mass");

	m_trader_flags.zero			();
	m_trader_flags.set			(eTraderFlagInfiniteAmmo,FALSE);
}

CSE_Abstract *CSE_ALifeTraderAbstract::init	()
{
	//string4096					S;
	//xr_sprintf						(S,"%s\r\n[game_info]\r\n",!*base()->m_ini_string ? "" : *base()->m_ini_string);
	//base()->m_ini_string		= S;

	return						(base());
}

CSE_ALifeTraderAbstract::~CSE_ALifeTraderAbstract()
{
}

void CSE_ALifeTraderAbstract::STATE_Write	(NET_Packet &tNetPacket)
{
	tNetPacket.w_u32			(m_dwMoney);

	tNetPacket.w_u32			(m_trader_flags.get());

	save_data					(m_character_name, tNetPacket);
}

void CSE_ALifeTraderAbstract::STATE_Read	(NET_Packet &tNetPacket, u16 size)
{
	shared_str sCharacterProfile;
	shared_str SpecificCharacter;

	tNetPacket.r_u32	(m_dwMoney);

	m_trader_flags.assign(tNetPacket.r_u32());

	load_data			(m_character_name, tNetPacket);

}

void CSE_ALifeTraderAbstract::OnChangeProfile(PropValue* sender)
{
	base()->set_editor_flag		(ISE_Abstract::flVisualChange);
}

#ifndef AI_COMPILER


#ifdef XRGAME_EXPORTS

#include "game_base_space.h"
#include "Level.h"

#endif


#endif


#ifdef XRGAME_EXPORTS

//для работы с relation system
u16								CSE_ALifeTraderAbstract::object_id		() const
{
	return base()->ID;
}


#endif

void CSE_ALifeTraderAbstract::UPDATE_Write	(NET_Packet &tNetPacket)
{
};

void CSE_ALifeTraderAbstract::UPDATE_Read	(NET_Packet &tNetPacket)
{
};

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeCustomZone
////////////////////////////////////////////////////////////////////////////
CSE_ALifeCustomZone::CSE_ALifeCustomZone	(LPCSTR caSection) : CSE_ALifeSpaceRestrictor(caSection)
{
	m_owner_id					= u32(-1);
//	m_maxPower					= pSettings->r_float(caSection,"min_start_power");
	if (pSettings->line_exist(caSection,"hit_type"))
		m_tHitType				= ALife::g_tfString2HitType(pSettings->r_string(caSection,"hit_type"));
	else
		m_tHitType				= ALife::eHitTypeMax;
	m_enabled_time				= 0;
	m_disabled_time				= 0;
	m_start_time_shift			= 0;

}

CSE_ALifeCustomZone::~CSE_ALifeCustomZone	()
{
}

void CSE_ALifeCustomZone::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	
	float tmp;
	tNetPacket.r_float			(tmp/*m_maxPower*/);

	tNetPacket.r_u32		(m_owner_id);

	tNetPacket.r_u32		(m_enabled_time);
	tNetPacket.r_u32		(m_disabled_time);
	tNetPacket.r_u32		(m_start_time_shift);
}

void CSE_ALifeCustomZone::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_float			(0.0/*m_maxPower*/);
	tNetPacket.w_u32			(m_owner_id);
	tNetPacket.w_u32			(m_enabled_time);
	tNetPacket.w_u32			(m_disabled_time);
	tNetPacket.w_u32			(m_start_time_shift);
}

void CSE_ALifeCustomZone::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeCustomZone::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeCustomZone::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps		(pref,items);
	PHelper().CreateU32			(items,PrepareKey(pref,*s_name,"on/off mode\\Shift time (sec)"),	&m_start_time_shift,0,100000);
	PHelper().CreateU32			(items,PrepareKey(pref,*s_name,"on/off mode\\Enabled time (sec)"),	&m_enabled_time,	0,100000);
	PHelper().CreateU32			(items,PrepareKey(pref,*s_name,"on/off mode\\Disabled time (sec)"),	&m_disabled_time,	0,100000);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeAnomalousZone
////////////////////////////////////////////////////////////////////////////
CSE_ALifeAnomalousZone::CSE_ALifeAnomalousZone(LPCSTR caSection) : CSE_ALifeCustomZone(caSection)
{
	m_offline_interactive_radius	= 30.f;
	m_artefact_spawn_count			= 32;
//	m_spawn_flags.set				(flSpawnDestroyOnSpawn,TRUE);
}

CSE_Abstract *CSE_ALifeAnomalousZone::init			()
{
	inherited::init				();
	return						(base());
}

CSE_Abstract *CSE_ALifeAnomalousZone::base			()
{
	return						(inherited::base());
}

const CSE_Abstract *CSE_ALifeAnomalousZone::base	() const
{
	return						(inherited::base());
}

CSE_ALifeAnomalousZone::~CSE_ALifeAnomalousZone		()
{
}


void CSE_ALifeAnomalousZone::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);

	tNetPacket.r_float		(m_offline_interactive_radius);
	
	tNetPacket.r_u16		(m_artefact_spawn_count);
	tNetPacket.r_u32		(m_artefact_position_offset);
}

void CSE_ALifeAnomalousZone::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_float			(m_offline_interactive_radius);
	tNetPacket.w_u16			(m_artefact_spawn_count);
	tNetPacket.w_u32			(m_artefact_position_offset);
}

void CSE_ALifeAnomalousZone::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeAnomalousZone::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write	(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeAnomalousZone::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
	PHelper().CreateFloat			(items,PrepareKey(pref,*s_name,"offline interactive radius"),			&m_offline_interactive_radius,	0.f,	100.f);
	PHelper().CreateU16				(items,PrepareKey(pref,*s_name,"ALife\\Artefact spawn places count"),	&m_artefact_spawn_count,		32,		256);
//	PHelper().CreateFlag32			(items,PrepareKey(pref,*s_name,"ALife\\Visible for AI"),				&m_flags,						flVisibleForAI);
}
#endif // #ifndef XRGAME_EXPORTS

//////////////////////////////////////////////////////////////////////////
//SE_ALifeTorridZone
//////////////////////////////////////////////////////////////////////////
CSE_ALifeTorridZone::CSE_ALifeTorridZone	(LPCSTR caSection)
:CSE_ALifeCustomZone(caSection),CSE_Motion()
{
}

CSE_ALifeTorridZone::~CSE_ALifeTorridZone	()
{
}

CSE_Motion* CSE_ALifeTorridZone::motion		()
{
	return						(this);
}

void CSE_ALifeTorridZone::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read		(tNetPacket,size);
	CSE_Motion::motion_read		(tNetPacket);
	set_editor_flag				(flMotionChange);
}

void CSE_ALifeTorridZone::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	CSE_Motion::motion_write	(tNetPacket);

}

void CSE_ALifeTorridZone::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeTorridZone::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeTorridZone::FillProps(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps		(pref, values);
	inherited2::FillProps		(pref, values);
}
#endif // #ifndef XRGAME_EXPORTS

//////////////////////////////////////////////////////////////////////////
//CSE_ALifeZoneVisual
//////////////////////////////////////////////////////////////////////////
CSE_ALifeZoneVisual::CSE_ALifeZoneVisual(LPCSTR caSection)
:CSE_ALifeAnomalousZone(caSection),
CSE_Visual(caSection)
{
	if (pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));
//	if(pSettings->line_exist(caSection,"blast_animation"))
//		attack_animation=pSettings->r_string(caSection,"blast_animation");
}

CSE_ALifeZoneVisual::~CSE_ALifeZoneVisual	()
{

}

CSE_Visual* CSE_ALifeZoneVisual::visual	()
{
	return		static_cast<CSE_Visual*>(this);
}
void CSE_ALifeZoneVisual::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read		(tNetPacket,size);
	visual_read					(tNetPacket,m_wVersion);
	tNetPacket.r_stringZ(startup_animation);
	tNetPacket.r_stringZ(attack_animation);
}

void CSE_ALifeZoneVisual::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	visual_write				(tNetPacket);
	tNetPacket.w_stringZ(startup_animation);
	tNetPacket.w_stringZ(attack_animation);
}

void CSE_ALifeZoneVisual::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeZoneVisual::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeZoneVisual::FillProps(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps		(pref, values);
	inherited2::FillProps		(pref, values);
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	PHelper().CreateChoose(values,	PrepareKey(pref,abstract->name(),"Attack animation"),	&attack_animation, smSkeletonAnims,0,(void*)*visual_name);
}
#endif // #ifndef XRGAME_EXPORTS
//-------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeCreatureAbstract
////////////////////////////////////////////////////////////////////////////
CSE_ALifeCreatureAbstract::CSE_ALifeCreatureAbstract(LPCSTR caSection)	: CSE_ALifeDynamicObjectVisual(caSection)
{
	s_team = s_squad = s_group	= 0;
	o_model						= 0.f;
	o_torso.pitch				= 0.f;
	o_torso.yaw					= 0.f;
	o_torso.roll				= 0.f;
	fHealth						= 1;
	m_bDeathIsProcessed			= false;
	m_fAccuracy					= 25.f;
	m_fIntelligence				= 25.f;
	m_fMorale					= 100.f;
	m_killer_id					= ALife::_OBJECT_ID(-1);
	m_game_death_time			= 0;
}

CSE_ALifeCreatureAbstract::~CSE_ALifeCreatureAbstract()
{
}

#ifdef DEBUG
bool CSE_ALifeCreatureAbstract::match_configuration	() const
{
	return						(!strstr(Core.Params,"-designer"));
}
#endif


void CSE_ALifeCreatureAbstract::STATE_Write	(NET_Packet &tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u8				(s_team	);
	tNetPacket.w_u8				(s_squad);
	tNetPacket.w_u8				(s_group);
	tNetPacket.w_float			(fHealth);
	save_data					(m_dynamic_out_restrictions,tNetPacket);
	save_data					(m_dynamic_in_restrictions,tNetPacket);
	tNetPacket.w_u16			( get_killer_id() );
	R_ASSERT(!(get_health() > 0.0f && get_killer_id() != u16(-1)));
	tNetPacket.w_u64			(m_game_death_time);
}

void CSE_ALifeCreatureAbstract::STATE_Read	(NET_Packet &tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);
	tNetPacket.r_u8				(s_team	);
	tNetPacket.r_u8				(s_squad);
	tNetPacket.r_u8				(s_group);
	tNetPacket.r_float		(fHealth);

	o_model						= o_torso.yaw;

	load_data				(m_dynamic_out_restrictions,tNetPacket);
	load_data				(m_dynamic_in_restrictions,tNetPacket);

	set_killer_id			( tNetPacket.r_u16() );

	o_torso.pitch			= o_Angle.x;
	o_torso.yaw				= o_Angle.y;

	tNetPacket.r_u64		(m_game_death_time);
}

void CSE_ALifeCreatureAbstract::UPDATE_Write(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
	
	tNetPacket.w_float			(fHealth);
	
	tNetPacket.w_u32			(timestamp		);
	tNetPacket.w_u8				(flags			);
	tNetPacket.w_vec3			(o_Position		);
	tNetPacket.w_float /*w_angle8*/			(o_model		);
	tNetPacket.w_float /*w_angle8*/			(o_torso.yaw	);
	tNetPacket.w_float /*w_angle8*/			(o_torso.pitch	);
	tNetPacket.w_float /*w_angle8*/			(o_torso.roll	);
	tNetPacket.w_u8				(s_team);
	tNetPacket.w_u8				(s_squad);
	tNetPacket.w_u8				(s_group);
};

void CSE_ALifeCreatureAbstract::UPDATE_Read	(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
	
	tNetPacket.r_float			(fHealth);
	
	tNetPacket.r_u32			(timestamp		);
	tNetPacket.r_u8				(flags			);
	tNetPacket.r_vec3			(o_Position		);
	tNetPacket.r_float /*r_angle8*/			(o_model		);
	tNetPacket.r_float /*r_angle8*/			(o_torso.yaw	);
	tNetPacket.r_float /*r_angle8*/			(o_torso.pitch	);
	tNetPacket.r_float /*r_angle8*/			(o_torso.roll	);
	
	tNetPacket.r_u8				(s_team);
	tNetPacket.r_u8				(s_squad);
	tNetPacket.r_u8				(s_group);
};

u8 CSE_ALifeCreatureAbstract::g_team		()
{
	return s_team;
}

u8 CSE_ALifeCreatureAbstract::g_squad		()
{
	return s_squad;
}

u8 CSE_ALifeCreatureAbstract::g_group		()
{
	return s_group;
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeCreatureAbstract::FillProps	(LPCSTR pref, PropItemVec& items)
{
  	inherited::FillProps			(pref,items);
    PHelper().CreateU8				(items,PrepareKey(pref,*s_name, "Team"),		&s_team, 	0,64,1);
    PHelper().CreateU8				(items,PrepareKey(pref,*s_name, "Squad"),	&s_squad, 	0,64,1);
    PHelper().CreateU8				(items,PrepareKey(pref,*s_name, "Group"),	&s_group, 	0,64,1);
   	PHelper().CreateFloat			(items,PrepareKey(pref,*s_name,"Personal",	"Health" 				),&fHealth,	0,2,5);
}
#endif // #ifndef XRGAME_EXPORTS

void CSE_ALifeCreatureAbstract::set_health	(float const health_value)
{
	VERIFY( !((get_killer_id() != u16(-1)) && (health_value > 0.f)) );
	fHealth = health_value;
}

void CSE_ALifeCreatureAbstract::set_killer_id	(ALife::_OBJECT_ID const killer_id)
{
	m_killer_id = killer_id;
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeCreatureActor
////////////////////////////////////////////////////////////////////////////

CSE_ALifeCreatureActor::CSE_ALifeCreatureActor	(LPCSTR caSection) 
: CSE_ALifeCreatureAbstract(caSection), CSE_ALifeTraderAbstract(caSection),CSE_PHSkeleton(caSection)
{
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));
	m_u16NumItems				= 0;
//	fArmor						= 0.f;
	fRadiation					= 0.f;
	accel.set					(0.f,0.f,0.f);
	velocity.set				(0.f,0.f,0.f);
	m_holderID					=u16(-1);
	mstate						= 0;
}

CSE_ALifeCreatureActor::~CSE_ALifeCreatureActor()
{
}

#ifdef DEBUG
bool CSE_ALifeCreatureActor::match_configuration	() const
{
	return						(true);
}
#endif

CSE_Abstract *CSE_ALifeCreatureActor::init			()
{
	inherited1::init			();
	inherited2::init			();
	return						(inherited1::base());
}

CSE_Abstract *CSE_ALifeCreatureActor::base			()
{
	return						(inherited1::base());
}

const CSE_Abstract *CSE_ALifeCreatureActor::base	() const
{
	return						(inherited1::base());
}

void CSE_ALifeCreatureActor::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read	(tNetPacket,size);
	inherited2::STATE_Read	(tNetPacket,size);
	inherited3::STATE_Read	(tNetPacket,size);
	m_holderID				=tNetPacket.r_u16();
};

void CSE_ALifeCreatureActor::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	inherited2::STATE_Write		(tNetPacket);
	inherited3::STATE_Write		(tNetPacket);
	tNetPacket.w_u16			(m_holderID);
};

BOOL CSE_ALifeCreatureActor::Net_Relevant()
{
	return TRUE; // this is a big question ;)
}

void CSE_ALifeCreatureActor::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);
	tNetPacket.r_u16			(mstate		);
	tNetPacket.r_sdir			(accel		);
	tNetPacket.r_sdir			(velocity	);
	tNetPacket.r_float			(fRadiation	);
	tNetPacket.r_u8				(weapon		);
	////////////////////////////////////////////////////
	tNetPacket.r_u16			(m_u16NumItems);

	if (!m_u16NumItems) return;

	if (m_u16NumItems == 1)
	{
		tNetPacket.r_u8					( *((u8*)&(m_AliveState.enabled)) );

		tNetPacket.r_vec3				( m_AliveState.angular_vel );
		tNetPacket.r_vec3				( m_AliveState.linear_vel );

		tNetPacket.r_vec3				( m_AliveState.force );
		tNetPacket.r_vec3				( m_AliveState.torque );

		tNetPacket.r_vec3				( m_AliveState.position );

		tNetPacket.r_float				( m_AliveState.quaternion.x );
		tNetPacket.r_float				( m_AliveState.quaternion.y );
		tNetPacket.r_float				( m_AliveState.quaternion.z );
		tNetPacket.r_float				( m_AliveState.quaternion.w );

		return;
	};	
	////////////// Import dead body ////////////////////
	Msg	("A mi ni hera tut ne chitaem (m_u16NumItems == %d)",m_u16NumItems);
	{
		m_BoneDataSize = tNetPacket.r_u8();
		u32 BodyDataSize = 24 + m_BoneDataSize*m_u16NumItems;
		tNetPacket.r(m_DeadBodyData, BodyDataSize);
	};
};
void CSE_ALifeCreatureActor::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
	inherited2::UPDATE_Write	(tNetPacket);
	tNetPacket.w_u16			(mstate		);
	tNetPacket.w_sdir			(accel		);
	tNetPacket.w_sdir			(velocity	);
	tNetPacket.w_float			(fRadiation	);
	tNetPacket.w_u8				(weapon		);
	////////////////////////////////////////////////////
	tNetPacket.w_u16			(m_u16NumItems);
	if (!m_u16NumItems) return;	

	if (m_u16NumItems == 1)
	{
		tNetPacket.w_u8					( m_AliveState.enabled );

		tNetPacket.w_vec3				( m_AliveState.angular_vel );
		tNetPacket.w_vec3				( m_AliveState.linear_vel );

		tNetPacket.w_vec3				( m_AliveState.force );
		tNetPacket.w_vec3				( m_AliveState.torque );

		tNetPacket.w_vec3				( m_AliveState.position );

		tNetPacket.w_float				( m_AliveState.quaternion.x );
		tNetPacket.w_float				( m_AliveState.quaternion.y );
		tNetPacket.w_float				( m_AliveState.quaternion.z );
		tNetPacket.w_float				( m_AliveState.quaternion.w );	

		return;
	};
	////////////// Export dead body ////////////////////
	{
		tNetPacket.w_u8(m_BoneDataSize);
		u32 BodyDataSize = 24 + m_BoneDataSize*m_u16NumItems;
		tNetPacket.w(m_DeadBodyData, BodyDataSize);
	};
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeCreatureActor::FillProps		(LPCSTR pref, PropItemVec& items)
{
  	inherited1::FillProps		(pref,items);
  	inherited2::FillProps		(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeCreatureCrow
////////////////////////////////////////////////////////////////////////////
CSE_ALifeCreatureCrow::CSE_ALifeCreatureCrow(LPCSTR caSection) : CSE_ALifeCreatureAbstract(caSection)
{
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));
//	m_flags.set					(flUseSwitches,FALSE);
//	m_flags.set					(flSwitchOffline,FALSE);
}

CSE_ALifeCreatureCrow::~CSE_ALifeCreatureCrow()
{
}

void CSE_ALifeCreatureCrow::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read	(tNetPacket,size);
}

void CSE_ALifeCreatureCrow::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeCreatureCrow::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeCreatureCrow::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeCreatureCrow::FillProps			(LPCSTR pref, PropItemVec& values)
{
  	inherited::FillProps			(pref,values);
}
#endif // #ifndef XRGAME_EXPORTS
