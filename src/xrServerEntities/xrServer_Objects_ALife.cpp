////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects for ALife simulator
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "game_base_space.h"
#include "restriction_space.h"

#ifndef XRGAME_EXPORTS
	LPCSTR GAME_CONFIG = "game.ltx";
#else // XRGAME_EXPORTS
#	include "../xrEngine/bone.h"
#	include "../xrEngine/render.h"
#endif // XRGAME_EXPORTS

#ifdef XRSE_FACTORY_EXPORTS
#	pragma warning(push)
#	pragma warning(disable:4995)
#		include <shlwapi.h>
#	pragma warning(pop)

#	pragma comment(lib, "shlwapi.lib")

struct logical_string_predicate {
	static HRESULT AnsiToUnicode						(LPCSTR pszA, LPVOID buffer, u32 const& buffer_size)
	{
		VERIFY			(pszA);
		VERIFY			(buffer);
		VERIFY			(buffer_size);

		u32				cCharacters =  xr_strlen(pszA)+1;
		VERIFY			(cCharacters*2 <= buffer_size);

		if (MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters, (LPOLESTR)buffer, cCharacters))
			return		(NOERROR);

		return			(HRESULT_FROM_WIN32(GetLastError()));
	}

	bool operator()			(LPCSTR const& first, LPCSTR const& second) const
	{
		u32				buffer_size0 = (xr_strlen(first) + 1)*2;
		LPCWSTR			buffer0 = (LPCWSTR)_alloca(buffer_size0);
		AnsiToUnicode	(first, (LPVOID)buffer0, buffer_size0);

		u32				buffer_size1 = (xr_strlen(second) + 1)*2;
		LPCWSTR			buffer1 = (LPCWSTR)_alloca(buffer_size1);
		AnsiToUnicode	(second, (LPVOID)buffer1, buffer_size1);

		return			(StrCmpLogicalW(buffer0, buffer1) < 0);
	}

	bool operator()			(shared_str const& first, shared_str const& second) const
	{
		u32				buffer_size0 = (first.size() + 1)*2;
		LPCWSTR			buffer0 = (LPCWSTR)_alloca(buffer_size0);
		AnsiToUnicode	(first.c_str(), (LPVOID)buffer0, buffer_size0);

		u32				buffer_size1 = (second.size() + 1)*2;
		LPCWSTR			buffer1 = (LPCWSTR)_alloca(buffer_size1);
		AnsiToUnicode	(second.c_str(), (LPVOID)buffer1, buffer_size1);

		return			(StrCmpLogicalW(buffer0, buffer1) < 0);
	}
}; // struct logical_string_predicate

#endif // XRSE_FACTORY_EXPORTS

bool SortStringsByAlphabetPred (const shared_str& s1, const shared_str& s2)
{
	R_ASSERT(s1.size());
	R_ASSERT(s2.size());

	return (xr_strcmp(s1,s2)<0);
};

struct story_name_predicate {
	IC	bool	operator()	(const xr_rtoken &_1, const xr_rtoken &_2) const
	{
		VERIFY	(_1.name.size());
		VERIFY	(_2.name.size());

		return	(xr_strcmp(_1.name,_2.name) < 0);
	}
};

#ifdef XRSE_FACTORY_EXPORTS
SFillPropData::SFillPropData	()
{
	counter = 0;
};

SFillPropData::~SFillPropData	()
{
	VERIFY	(0==counter);
};

void	SFillPropData::load			()
    {
      // create ini
#ifdef XRGAME_EXPORTS
    CInifile				*Ini = 	pGameIni;
#else // XRGAME_EXPORTS
    CInifile				*Ini = 0;
    string_path				gm_name;
    FS.update_path			(gm_name,"$game_config$",GAME_CONFIG);
    R_ASSERT3				(FS.exist(gm_name),"Couldn't find file",gm_name);
    Ini						= xr_new<CInifile>(gm_name);
#endif // XRGAME_EXPORTS

    // location type
    LPCSTR					N,V;
    u32 					k;
	for (int i=0; i<GameGraph::LOCATION_TYPE_COUNT; ++i){
        VERIFY				(locations[i].empty());
        string256			caSection, T;
        strconcat			(sizeof(caSection),caSection,SECTION_HEADER,itoa(i,T,10));
        R_ASSERT			(Ini->section_exist(caSection));
        for (k = 0; Ini->r_line(caSection,k,&N,&V); ++k)
            locations[i].push_back	(xr_rtoken(V,atoi(N)));
    }
    for (k = 0; Ini->r_line("graph_points_draw_color_palette",k,&N,&V); ++k)
	{
		u32 color;
		if(1==sscanf(V,"%x", &color))
		{
			location_colors[N]  = color;
		}else
			Msg("! invalid record format in [graph_points_draw_color_palette] %s=%s",N,V);
	}
    
	// level names/ids
    VERIFY					(level_ids.empty());
    for (k = 0; Ini->r_line("levels",k,&N,&V); ++k)
        level_ids.push_back	(Ini->r_string_wb(N,"caption"));

    // story names
	{
		VERIFY					(story_names.empty());
		LPCSTR section 			= "story_ids";
		R_ASSERT				(Ini->section_exist(section));
		for (k = 0; Ini->r_line(section,k,&N,&V); ++k)
			story_names.push_back	(xr_rtoken(V,atoi(N)));

		std::sort				(story_names.begin(),story_names.end(),story_name_predicate());
		story_names.insert		(story_names.begin(),xr_rtoken("NO STORY ID",ALife::_STORY_ID(-1)));
	}

    // spawn story names
	{
		VERIFY					(spawn_story_names.empty());
		LPCSTR section 			= "spawn_story_ids";
		R_ASSERT				(Ini->section_exist(section));
		for (k = 0; Ini->r_line(section,k,&N,&V); ++k)
			spawn_story_names.push_back	(xr_rtoken(V,atoi(N)));

		std::sort				(spawn_story_names.begin(),spawn_story_names.end(),story_name_predicate());
		spawn_story_names.insert(spawn_story_names.begin(),xr_rtoken("NO SPAWN STORY ID",ALife::_SPAWN_STORY_ID(-1)));
	}

	
    // destroy ini
#ifndef XRGAME_EXPORTS
	xr_delete				(Ini);
#endif // XRGAME_EXPORTS
};

void	SFillPropData::unload			()
{
    for (int i=0; i<GameGraph::LOCATION_TYPE_COUNT; ++i)
        locations[i].clear	();
    level_ids.clear			();
    story_names.clear		();
    spawn_story_names.clear	();
	character_profiles.clear();
	smart_covers.clear		();
};

void	SFillPropData::dec				()
{
    VERIFY					(counter > 0);
    --counter;

    if (!counter)
        unload				();
};

void	SFillPropData::inc				()
{
    VERIFY					(counter < 0xffffffff);

    if (!counter)
        load				();

    ++counter;
}
static SFillPropData			fp_data;
#endif // #ifdef XRSE_FACTORY_EXPORTS

#ifndef XRGAME_EXPORTS
void CSE_ALifeTraderAbstract::FillProps	(LPCSTR pref, PropItemVec& items)
{
#	ifdef XRSE_FACTORY_EXPORTS
	PHelper().CreateU32			(items, PrepareKey(pref,*base()->s_name,"Money"), 	&m_dwMoney,	0, u32(-1));
	PHelper().CreateFlag32		(items,	PrepareKey(pref,*base()->s_name,"Trader\\Infinite ammo"),&m_trader_flags, eTraderFlagInfiniteAmmo);
#	endif // #ifdef XRSE_FACTORY_EXPORTS
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeDynamicObjectVisual
////////////////////////////////////////////////////////////////////////////
CSE_ALifeDynamicObjectVisual::CSE_ALifeDynamicObjectVisual(LPCSTR caSection) 
:inherited1(caSection),
inherited2()
{
	if (pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));
}

CSE_ALifeDynamicObjectVisual::~CSE_ALifeDynamicObjectVisual()
{
}

CSE_Visual* CSE_ALifeDynamicObjectVisual::visual	()
{
	return						(this);
}

void CSE_ALifeDynamicObjectVisual::STATE_Write(NET_Packet &tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	visual_write				(tNetPacket);
}

void CSE_ALifeDynamicObjectVisual::STATE_Read(NET_Packet &tNetPacket, u16 size)
{
	inherited1::STATE_Read	(tNetPacket, size);
	visual_read				(tNetPacket,m_wVersion);
}

void CSE_ALifeDynamicObjectVisual::UPDATE_Write(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
};

void CSE_ALifeDynamicObjectVisual::UPDATE_Read(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
};

#ifndef XRGAME_EXPORTS
void CSE_ALifeDynamicObjectVisual::FillProps	(LPCSTR pref, PropItemVec& items)
{
	inherited1::FillProps		(pref,items);
	inherited2::FillProps		(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifePHSkeletonObject
////////////////////////////////////////////////////////////////////////////
CSE_ALifePHSkeletonObject::CSE_ALifePHSkeletonObject(LPCSTR caSection) 
:inherited1(caSection),
inherited2(caSection)
{
}

CSE_ALifePHSkeletonObject::~CSE_ALifePHSkeletonObject()
{
}


void CSE_ALifePHSkeletonObject::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read(tNetPacket,size);
	inherited2::STATE_Read(tNetPacket,size);

}

void CSE_ALifePHSkeletonObject::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	inherited2::STATE_Write		(tNetPacket);
}

void CSE_ALifePHSkeletonObject::UPDATE_Write(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
	inherited2::UPDATE_Write	(tNetPacket);
};

void CSE_ALifePHSkeletonObject::UPDATE_Read(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);
};

bool CSE_ALifePHSkeletonObject::can_save			() const
{
	return						CSE_PHSkeleton::need_save();
}

#ifndef XRGAME_EXPORTS
void CSE_ALifePHSkeletonObject::FillProps(LPCSTR pref, PropItemVec& items)
{
	inherited1::FillProps			(pref,items);
	inherited2::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeSpaceRestrictor
////////////////////////////////////////////////////////////////////////////
CSE_ALifeSpaceRestrictor::CSE_ALifeSpaceRestrictor(LPCSTR caSection) 
:inherited1(caSection),
inherited2()
{
	m_space_restrictor_type		= RestrictionSpace::eDefaultRestrictorTypeNone;
}

CSE_ALifeSpaceRestrictor::~CSE_ALifeSpaceRestrictor	()
{
}

ISE_Shape* CSE_ALifeSpaceRestrictor::shape		()
{
	return						(this);
}

void CSE_ALifeSpaceRestrictor::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read		(tNetPacket,size);
	cform_read					(tNetPacket);
	m_space_restrictor_type		= tNetPacket.r_u8();
}

void CSE_ALifeSpaceRestrictor::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	cform_write					(tNetPacket);
	tNetPacket.w_u8				(m_space_restrictor_type);
}

void CSE_ALifeSpaceRestrictor::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeSpaceRestrictor::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
}

xr_token defaul_retrictor_types[]={
	{ "NOT A restrictor",			RestrictionSpace::eRestrictorTypeNone},
	{ "NONE default restrictor",	RestrictionSpace::eDefaultRestrictorTypeNone},
	{ "OUT default restrictor",		RestrictionSpace::eDefaultRestrictorTypeOut	},
	{ "IN default restrictor",		RestrictionSpace::eDefaultRestrictorTypeIn	},
	{ 0,							0}
};

#ifndef XRGAME_EXPORTS
void CSE_ALifeSpaceRestrictor::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited1::FillProps		(pref,items);
	PHelper().CreateToken8		(items, PrepareKey(pref,*s_name,"restrictor type"),		&m_space_restrictor_type,	defaul_retrictor_types);
//	PHelper().CreateFlag32		(items,	PrepareKey(pref,*s_name,"check for separator"),	&m_flags,					flCheckForSeparator);
}
#endif // #ifndef XRGAME_EXPORTS


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeObjectPhysic
////////////////////////////////////////////////////////////////////////////
CSE_ALifeObjectPhysic::CSE_ALifeObjectPhysic(LPCSTR caSection) : CSE_ALifeDynamicObjectVisual(caSection), CSE_PHSkeleton(caSection)
{
	type 						= epotSkeleton;
	mass 						= 10.f;

	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
	{
    	set_visual				(pSettings->r_string(caSection,"visual"));
		
		if(pSettings->line_exist(caSection,"startup_animation"))
			startup_animation		= pSettings->r_string(caSection,"startup_animation");
	}
	
	if(pSettings->line_exist(caSection,"fixed_bones"))
		fixed_bones		= pSettings->r_string(caSection,"fixed_bones");
	
#ifdef XRGAME_EXPORTS
	m_freeze_time				= Device.dwTimeGlobal;
#	ifdef DEBUG
	m_last_update_time			= Device.dwTimeGlobal;
#	endif
#else
	m_freeze_time				= 0;
#endif
	m_relevent_random.seed		(u32(CPU::GetCLK() & u32(-1)));
}

CSE_ALifeObjectPhysic::~CSE_ALifeObjectPhysic		() 
{
}

void CSE_ALifeObjectPhysic::STATE_Read		(NET_Packet	&tNetPacket, u16 size) 
{
	inherited1::STATE_Read(tNetPacket,size);
	inherited2::STATE_Read(tNetPacket,size);
		
	tNetPacket.r_u32			(type);
	tNetPacket.r_float			(mass);
    
	tNetPacket.r_stringZ		(fixed_bones);

	set_editor_flag				(flVisualAnimationChange);
}

void CSE_ALifeObjectPhysic::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	inherited2::STATE_Write		(tNetPacket);
	tNetPacket.w_u32			(type);
	tNetPacket.w_float			(mass);
	tNetPacket.w_stringZ		(fixed_bones);

}

static inline bool check (const u8 &mask, const u8 &test)
{
	return							(!!(mask & test));
}

const	u32		CSE_ALifeObjectPhysic::m_freeze_delta_time		= 5000;
const	u32		CSE_ALifeObjectPhysic::random_limit				= 40;		

#ifdef DEBUG
const	u32		CSE_ALifeObjectPhysic::m_update_delta_time		= 0;
#endif // #ifdef DEBUG

//if TRUE, then object sends update packet
BOOL CSE_ALifeObjectPhysic::Net_Relevant()
{
	if (!freezed)
	{
#ifdef XRGAME_EXPORTS
#ifdef DEBUG	//this block of code is only for test
		if (Device.dwTimeGlobal < (m_last_update_time + m_update_delta_time))
			return FALSE;
#endif
#endif
		return		TRUE;
	}

#ifdef XRGAME_EXPORTS
	if (Device.dwTimeGlobal >= (m_freeze_time + m_freeze_delta_time))
		return		FALSE;
#endif
	if (!prev_freezed)
	{
		prev_freezed = true;	//i.e. freezed
		return		TRUE;
	}

	if (m_relevent_random.randI(random_limit))
		return		FALSE;

	return			TRUE;
}

void CSE_ALifeObjectPhysic::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);

	if (tNetPacket.r_eof())		//backward compatibility
		return;

//////////////////////////////////////////////////////////////////////////
	tNetPacket.r_u8					(m_u8NumItems);
	if (!m_u8NumItems) {
		return;
	}

	mask_num_items					num_items;
	num_items.common				= m_u8NumItems;
	m_u8NumItems					= num_items.num_items;

	R_ASSERT2						(
		m_u8NumItems < (u8(1) << 5),
		make_string("%d",m_u8NumItems)
		);
	
	{
		tNetPacket.r_vec3				(State.force);
		tNetPacket.r_vec3				(State.torque);

		tNetPacket.r_vec3				(State.position);

		tNetPacket.r_float			(State.quaternion.x);
		tNetPacket.r_float			(State.quaternion.y);
		tNetPacket.r_float			(State.quaternion.z);
		tNetPacket.r_float			(State.quaternion.w);	

		State.enabled					= check(num_items.mask,inventory_item_state_enabled);

		if (!check(num_items.mask,inventory_item_angular_null)) {
			tNetPacket.r_float		(State.angular_vel.x);
			tNetPacket.r_float		(State.angular_vel.y);
			tNetPacket.r_float		(State.angular_vel.z);
		}
		else
			State.angular_vel.set		(0.f,0.f,0.f);

		if (!check(num_items.mask,inventory_item_linear_null)) {
			tNetPacket.r_float		(State.linear_vel.x);
			tNetPacket.r_float		(State.linear_vel.y);
			tNetPacket.r_float		(State.linear_vel.z);
		}
		else
			State.linear_vel.set		(0.f,0.f,0.f);
	}

	prev_freezed = freezed;
	if (tNetPacket.r_eof())		// in case spawn + update 
	{
		freezed = false;
		return;
	}
	if (tNetPacket.r_u8())
	{
		freezed = false;
	}
	else {
		if (!freezed)
#ifdef XRGAME_EXPORTS
			m_freeze_time	= Device.dwTimeGlobal;
#else
			m_freeze_time	= 0;
#endif
		freezed = true;
	}
}

void CSE_ALifeObjectPhysic::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write		(tNetPacket);
	inherited2::UPDATE_Write		(tNetPacket);
//////////////////////////////////////////////////////////////////////////
	if (!m_u8NumItems) {
		tNetPacket.w_u8				(0);
		return;
	}

	mask_num_items					num_items;
	num_items.mask					= 0;
	num_items.num_items				= m_u8NumItems;

	R_ASSERT2						(
		num_items.num_items < (u8(1) << 5),
		make_string("%d",num_items.num_items)
		);

	if (State.enabled)									num_items.mask |= inventory_item_state_enabled;
	if (fis_zero(State.angular_vel.square_magnitude()))	num_items.mask |= inventory_item_angular_null;
	if (fis_zero(State.linear_vel.square_magnitude()))	num_items.mask |= inventory_item_linear_null;
	//if (anim_use)										num_items.mask |= animated;

	tNetPacket.w_u8					(num_items.common);

	{
		tNetPacket.w_vec3				(State.force);
		tNetPacket.w_vec3				(State.torque);

		tNetPacket.w_vec3				(State.position);

		tNetPacket.w_float			(State.quaternion.x);
		tNetPacket.w_float			(State.quaternion.y);
		tNetPacket.w_float			(State.quaternion.z);
		tNetPacket.w_float			(State.quaternion.w);	

		if (!check(num_items.mask,inventory_item_angular_null)) {
			tNetPacket.w_float		(State.angular_vel.x);
			tNetPacket.w_float		(State.angular_vel.y);
			tNetPacket.w_float		(State.angular_vel.z);
		}

		if (!check(num_items.mask,inventory_item_linear_null)) {
			tNetPacket.w_float		(State.linear_vel.x);
			tNetPacket.w_float		(State.linear_vel.y);
			tNetPacket.w_float		(State.linear_vel.z);
		}

	}
	tNetPacket.w_u8(1);	//not freezed - doesn't mean anything..

#ifdef XRGAME_EXPORTS
#ifdef DEBUG
	m_last_update_time			= Device.dwTimeGlobal;
#endif
#endif
}


xr_token po_types[]={
	{ "Box",			epotBox			},
	{ "Fixed chain",	epotFixedChain	},
	{ "Free chain",		epotFreeChain	},
	{ "Skeleton",		epotSkeleton	},
	{ 0,				0				}
};

#ifndef XRGAME_EXPORTS
void CSE_ALifeObjectPhysic::FillProps		(LPCSTR pref, PropItemVec& values) 
{
	inherited1::FillProps		(pref,	 values);
	inherited2::FillProps		(pref,	 values);

	PHelper().CreateToken32		(values, PrepareKey(pref,*s_name,"Type"), &type,	po_types);
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Mass"), &mass, 0.1f, 10000.f);
    PHelper().CreateFlag8		(values, PrepareKey(pref,*s_name,"Active"), &_flags, flActive);

    // motions & bones
	PHelper().CreateChoose		(values, 	PrepareKey(pref,*s_name,"Model\\Fixed bones"),	&fixed_bones,		smSkeletonBones,0,(void*)visual()->get_visual(),8);
}
#endif // #ifndef XRGAME_EXPORTS

bool CSE_ALifeObjectPhysic::can_save			() const
{
		return						CSE_PHSkeleton::need_save();
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeObjectHangingLamp
////////////////////////////////////////////////////////////////////////////
CSE_ALifeObjectHangingLamp::CSE_ALifeObjectHangingLamp(LPCSTR caSection) 
:inherited1(caSection),
inherited2(caSection)
{
	flags.assign				(flTypeSpot|flR1|flR2);

	range						= 10.f;
	color						= 0xffffffff;
    brightness					= 1.f;
	m_health					= 100.f;

	m_virtual_size				= 0.1f;
	m_ambient_radius			= 10.f;
    m_ambient_power				= 0.1f;
    spot_cone_angle				= deg2rad(120.f);
    glow_radius					= 0.7f;
	m_volumetric_quality		= 1.0f;
	m_volumetric_intensity		= 1.0f;
	m_volumetric_distance		= 1.0f;
}

CSE_ALifeObjectHangingLamp::~CSE_ALifeObjectHangingLamp()
{
}

void CSE_ALifeObjectHangingLamp::STATE_Read	(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read	(tNetPacket,size);
	inherited2::STATE_Read	(tNetPacket,size);

	// model
	tNetPacket.r_u32			(color);
	tNetPacket.r_float			(brightness);
	tNetPacket.r_stringZ		(color_animator);
	tNetPacket.r_float			(range);
	tNetPacket.r_u16			(flags.flags);
	tNetPacket.r_stringZ		(startup_animation);
	set_editor_flag				(flVisualAnimationChange);
	tNetPacket.r_stringZ		(fixed_bones);
	tNetPacket.r_float			(m_health);

	tNetPacket.r_float			(m_virtual_size);
    tNetPacket.r_float			(m_ambient_radius);
	tNetPacket.r_float			(m_ambient_power);
    tNetPacket.r_stringZ		(m_ambient_texture);
    tNetPacket.r_stringZ		(light_texture);
    tNetPacket.r_stringZ		(light_main_bone);
    tNetPacket.r_float			(spot_cone_angle);
    tNetPacket.r_stringZ		(glow_texture);
    tNetPacket.r_float			(glow_radius);

	tNetPacket.r_stringZ		(light_ambient_bone);

	tNetPacket.r_float			(m_volumetric_quality);
	tNetPacket.r_float			(m_volumetric_intensity);
	tNetPacket.r_float			(m_volumetric_distance);
}

void CSE_ALifeObjectHangingLamp::STATE_Write(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	inherited2::STATE_Write		(tNetPacket);

	// model
	tNetPacket.w_u32			(color);
	tNetPacket.w_float			(brightness);
	tNetPacket.w_stringZ		(color_animator);
	tNetPacket.w_float			(range);
   	tNetPacket.w_u16			(flags.flags);
	tNetPacket.w_stringZ		(startup_animation);
    tNetPacket.w_stringZ		(fixed_bones);
	tNetPacket.w_float			(m_health);
	tNetPacket.w_float			(m_virtual_size);
    tNetPacket.w_float			(m_ambient_radius);
    tNetPacket.w_float			(m_ambient_power);
    tNetPacket.w_stringZ		(m_ambient_texture);

    tNetPacket.w_stringZ		(light_texture);
    tNetPacket.w_stringZ		(light_main_bone);
    tNetPacket.w_float			(spot_cone_angle);
    tNetPacket.w_stringZ		(glow_texture);
    tNetPacket.w_float			(glow_radius);
    
	tNetPacket.w_stringZ		(light_ambient_bone);

	tNetPacket.w_float			(m_volumetric_quality);
	tNetPacket.w_float			(m_volumetric_intensity);
	tNetPacket.w_float			(m_volumetric_distance);
}


void CSE_ALifeObjectHangingLamp::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeObjectHangingLamp::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited1::UPDATE_Write		(tNetPacket);
	inherited2::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeObjectHangingLamp::OnChangeFlag(PropValue* sender)
{
	set_editor_flag				(flUpdateProperties);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeObjectHangingLamp::FillProps	(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps		(pref,values);
	inherited2::FillProps		(pref,values);

    PropValue* P				= 0;
	PHelper().CreateFlag16		(values, PrepareKey(pref,*s_name,"Flags\\Physic"),		&flags,			flPhysic);
	PHelper().CreateFlag16		(values, PrepareKey(pref,*s_name,"Flags\\Cast Shadow"),	&flags,			flCastShadow);
	PHelper().CreateFlag16		(values, PrepareKey(pref,*s_name,"Flags\\Allow R1"),	&flags,			flR1);
	PHelper().CreateFlag16		(values, PrepareKey(pref,*s_name,"Flags\\Allow R2"),	&flags,			flR2);
	P=PHelper().CreateFlag16	(values, PrepareKey(pref,*s_name,"Flags\\Allow Ambient"),&flags,			flPointAmbient);
    P->OnChangeEvent.bind		(this,&CSE_ALifeObjectHangingLamp::OnChangeFlag);
	// 
	P=PHelper().CreateFlag16	(values, PrepareKey(pref,*s_name,"Light\\Type"), 		&flags,				flTypeSpot, "Point", "Spot");
    P->OnChangeEvent.bind		(this,&CSE_ALifeObjectHangingLamp::OnChangeFlag);
	PHelper().CreateColor		(values, PrepareKey(pref,*s_name,"Light\\Main\\Color"),			&color);
    PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Light\\Main\\Brightness"),	&brightness,		0.1f, 5.f);
	PHelper().CreateChoose		(values, PrepareKey(pref,*s_name,"Light\\Main\\Color Animator"),&color_animator, 	smLAnim);
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Light\\Main\\Range"),			&range,				0.1f, 1000.f);
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Light\\Main\\Virtual Size"),	&m_virtual_size,	0.f, 100.f);
	PHelper().CreateChoose		(values, PrepareKey(pref,*s_name,"Light\\Main\\Texture"),	    &light_texture, 	smTexture, "lights");
	PHelper().CreateChoose		(values, PrepareKey(pref,*s_name,"Light\\Main\\Bone"),			&light_main_bone,	smSkeletonBones,0,(void*)visual()->get_visual());
	if (flags.is(flTypeSpot))
	{
		PHelper().CreateAngle	(values, PrepareKey(pref,*s_name,"Light\\Main\\Cone Angle"),	&spot_cone_angle,	deg2rad(1.f), deg2rad(120.f));
//		PHelper().CreateFlag16	(values, PrepareKey(pref,*s_name,"Light\\Main\\Volumetric"),	&flags,			flVolumetric);
		P=PHelper().CreateFlag16	(values, PrepareKey(pref,*s_name,"Flags\\Volumetric"),	&flags,			flVolumetric);
		P->OnChangeEvent.bind	(this,&CSE_ALifeObjectHangingLamp::OnChangeFlag);
	}

	if (flags.is(flPointAmbient)){
		PHelper().CreateFloat	(values, PrepareKey(pref,*s_name,"Light\\Ambient\\Radius"),		&m_ambient_radius,	0.f, 1000.f);
		PHelper().CreateFloat	(values, PrepareKey(pref,*s_name,"Light\\Ambient\\Power"),		&m_ambient_power);
		PHelper().CreateChoose	(values, PrepareKey(pref,*s_name,"Light\\Ambient\\Texture"),	&m_ambient_texture,	smTexture, 	"lights");
		PHelper().CreateChoose	(values, PrepareKey(pref,*s_name,"Light\\Ambient\\Bone"),		&light_ambient_bone,smSkeletonBones,0,(void*)visual()->get_visual());
	}

	if (flags.is(flVolumetric))
	{
		PHelper().CreateFloat	(values, PrepareKey(pref,*s_name,"Light\\Volumetric\\Quality"),		&m_volumetric_quality,	0.f, 1.f);
		PHelper().CreateFloat	(values, PrepareKey(pref,*s_name,"Light\\Volumetric\\Intensity"),	&m_volumetric_intensity,0.f, 10.f);
		PHelper().CreateFloat	(values, PrepareKey(pref,*s_name,"Light\\Volumetric\\Distance"),	&m_volumetric_distance,	0.f, 1.f);
	}

	// fixed bones
    PHelper().CreateChoose		(values, PrepareKey(pref,*s_name,"Model\\Fixed bones"),	&fixed_bones,		smSkeletonBones,0,(void*)visual()->get_visual(),8);
    // glow
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Glow\\Radius"),	    &glow_radius,		0.01f, 100.f);
	PHelper().CreateChoose		(values, PrepareKey(pref,*s_name,"Glow\\Texture"),	    &glow_texture, 		smTexture,	"glow");
	// game
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Game\\Health"),		&m_health,			0.f, 100.f);
}

#define VIS_RADIUS 		0.25f
void CSE_ALifeObjectHangingLamp::on_render(CDUInterface* du, ISE_AbstractLEOwner* owner, bool bSelected, const Fmatrix& parent,int priority, bool strictB2F)
{
	inherited1::on_render		(du,owner,bSelected,parent,priority,strictB2F);
	if ((1==priority)&&(false==strictB2F)){
		u32 clr					= bSelected?0x00FFFFFF:0x00FFFF00;
		Fmatrix main_xform, ambient_xform;
		owner->get_bone_xform		(*light_main_bone,main_xform);
		main_xform.mulA_43			(parent);
		if(flags.is(flPointAmbient) ){
			owner->get_bone_xform	(*light_ambient_bone,ambient_xform);
			ambient_xform.mulA_43	(parent);
		}
		if (bSelected){
			if (flags.is(flTypeSpot)){
				du->DrawSpotLight	(main_xform.c, main_xform.k, range, spot_cone_angle, clr);
			}else{
				du->DrawLineSphere	(main_xform.c, range, clr, true);
			}
			if(flags.is(flPointAmbient) )
				du->DrawLineSphere	(ambient_xform.c, m_ambient_radius, clr, true);
		}
		du->DrawPointLight		(main_xform.c,VIS_RADIUS, clr);
		if(flags.is(flPointAmbient) )
			du->DrawPointLight	(ambient_xform.c,VIS_RADIUS, clr);
	}
}
#endif // #ifndef XRGAME_EXPORTS

bool CSE_ALifeObjectHangingLamp::validate( )
{
	if (flags.test(flR1) || flags.test(flR2))
		return					(true);

	Msg							("! Render type is not set properly!");
	return						(false);
}

bool CSE_ALifeObjectHangingLamp::match_configuration() const
{
	R_ASSERT3(flags.test(flR1) || flags.test(flR2),"no renderer type set for hanging-lamp ",name_replace());
#ifdef XRGAME_EXPORTS
	return						(
		(flags.test(flR1) && (::Render->get_generation() == IRender_interface::GENERATION_R1)) ||
		(flags.test(flR2) && (::Render->get_generation() == IRender_interface::GENERATION_R2))
	);
#else
	return						(true);
#endif
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeObjectSearchlight
////////////////////////////////////////////////////////////////////////////

CSE_ALifeObjectProjector::CSE_ALifeObjectProjector(LPCSTR caSection) 
:inherited(caSection)
{
	//m_flags.set					(flUseSwitches,FALSE);
	//m_flags.set					(flSwitchOffline,FALSE);
}

CSE_ALifeObjectProjector::~CSE_ALifeObjectProjector()
{
}

void CSE_ALifeObjectProjector::STATE_Read	(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read	(tNetPacket,size);
}

void CSE_ALifeObjectProjector::STATE_Write(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeObjectProjector::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeObjectProjector::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeObjectProjector::FillProps			(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps			(pref,	 values);
}
#endif // #ifndef XRGAME_EXPORTS


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeObjectBreakable
////////////////////////////////////////////////////////////////////////////
CSE_ALifeObjectBreakable::CSE_ALifeObjectBreakable	(LPCSTR caSection) 
:CSE_ALifeDynamicObjectVisual(caSection)
{
	m_health					= 1.f;
	//m_flags.set					(flUseSwitches,FALSE);
	//m_flags.set					(flSwitchOffline,FALSE);
}

CSE_ALifeObjectBreakable::~CSE_ALifeObjectBreakable	()
{
}

void CSE_ALifeObjectBreakable::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	tNetPacket.r_float			(m_health);
}

void CSE_ALifeObjectBreakable::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_float			(m_health);
}

void CSE_ALifeObjectBreakable::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeObjectBreakable::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeObjectBreakable::FillProps		(LPCSTR pref, PropItemVec& values)
{
  	inherited::FillProps			(pref,values);
	PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Health"),			&m_health,			0.f, 100.f);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeObjectClimable
////////////////////////////////////////////////////////////////////////////
CSE_ALifeObjectClimable::CSE_ALifeObjectClimable(LPCSTR caSection)
:inherited1(caSection),
inherited2()
{
	material  = "materials\\fake_ladders";
}

CSE_ALifeObjectClimable::~CSE_ALifeObjectClimable	()
{
}

ISE_Shape* CSE_ALifeObjectClimable::shape					()
{
	return						(this);
}

void CSE_ALifeObjectClimable::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited1::STATE_Read	(tNetPacket,size);
	cform_read				(tNetPacket);
	tNetPacket.r_stringZ	(material);
}

void CSE_ALifeObjectClimable::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited1::STATE_Write	(tNetPacket);
	cform_write				(tNetPacket);
	tNetPacket.w_stringZ	(material);
}

void CSE_ALifeObjectClimable::UPDATE_Read	(NET_Packet	&tNetPacket)
{
}

void CSE_ALifeObjectClimable::UPDATE_Write	(NET_Packet	&tNetPacket)
{
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeObjectClimable::FillProps		(LPCSTR pref, PropItemVec& values)
{
	//inherited1::FillProps			(pref,values);
	inherited1::FillProps			(pref,values);
	//PHelper().CreateFloat		(values, PrepareKey(pref,*s_name,"Health"),			&m_health,			0.f, 100.f);
}

void CSE_ALifeObjectClimable::set_additional_info(void* info)
{
	LPCSTR material_name = (LPCSTR)info;
	material				= material_name;
}
#endif // #ifndef XRGAME_EXPORTS


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeTeamBaseZone
////////////////////////////////////////////////////////////////////////////
CSE_ALifeTeamBaseZone::CSE_ALifeTeamBaseZone(LPCSTR caSection) : CSE_ALifeSpaceRestrictor(caSection)
{
	m_team						= 0;
}

CSE_ALifeTeamBaseZone::~CSE_ALifeTeamBaseZone()
{
}

void CSE_ALifeTeamBaseZone::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	tNetPacket.r_u8				(m_team);
}

void CSE_ALifeTeamBaseZone::STATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u8				(m_team);
}

void CSE_ALifeTeamBaseZone::UPDATE_Read	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeTeamBaseZone::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeTeamBaseZone::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps		(pref,items);
	PHelper().CreateU8			(items, PrepareKey(pref,*s_name,"team"),			&m_team,			0, 16);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeSmartZone
////////////////////////////////////////////////////////////////////////////

//CSE_ALifeSmartZone::CSE_ALifeSmartZone	(LPCSTR caSection) : CSE_ALifeSpaceRestrictor(caSection), CSE_ALifeSchedulable(caSection)
//{
//}
//
//CSE_ALifeSmartZone::~CSE_ALifeSmartZone	()
//{
//}
//
//CSE_Abstract *CSE_ALifeSmartZone::base		()
//{
//	return						(this);
//}
//
//const CSE_Abstract *CSE_ALifeSmartZone::base	() const
//{
//	return						(this);
//}
//
//CSE_Abstract *CSE_ALifeSmartZone::init		()
//{
//	inherited1::init			();
//	inherited2::init			();
//	return						(this);
//}
//
//void CSE_ALifeSmartZone::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
//{
//	inherited1::STATE_Read		(tNetPacket,size);
//}
//
//void CSE_ALifeSmartZone::STATE_Write	(NET_Packet	&tNetPacket)
//{
//	inherited1::STATE_Write		(tNetPacket);
//}
//
//void CSE_ALifeSmartZone::UPDATE_Read	(NET_Packet	&tNetPacket)
//{
//	inherited1::UPDATE_Read		(tNetPacket);
//}
//
//void CSE_ALifeSmartZone::UPDATE_Write	(NET_Packet	&tNetPacket)
//{
//	inherited1::UPDATE_Write	(tNetPacket);
//}
//
//#ifndef XRGAME_EXPORTS
//void CSE_ALifeSmartZone::FillProps		(LPCSTR pref, PropItemVec& items)
//{
//	inherited1::FillProps		(pref,items);
//}
//#endif // #ifndef XRGAME_EXPORTS
//
//void CSE_ALifeSmartZone::update			()
//{
//}
//
//float CSE_ALifeSmartZone::detect_probability()
//{
//	return						(0.f);
//}
//
//void CSE_ALifeSmartZone::smart_touch	(CSE_ALifeMonsterAbstract *monster)
//{
//}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeInventoryBox
////////////////////////////////////////////////////////////////////////////

CSE_ALifeInventoryBox::CSE_ALifeInventoryBox( LPCSTR caSection ) : CSE_ALifeDynamicObjectVisual( caSection )
{
	m_can_take = true;
	m_closed   = false;
	m_tip_text._set( "inventory_box_use" );
}

CSE_ALifeInventoryBox::~CSE_ALifeInventoryBox()
{
}

void CSE_ALifeInventoryBox::STATE_Read( NET_Packet &tNetPacket, u16 size )
{
	inherited::STATE_Read( tNetPacket, size );

	u8 temp;
	tNetPacket.r_u8	( temp );		m_can_take = (temp == 1);
	tNetPacket.r_u8	( temp );		m_closed   = (temp == 1);
	tNetPacket.r_stringZ( m_tip_text );
}

void CSE_ALifeInventoryBox::STATE_Write( NET_Packet &tNetPacket )
{
	inherited::STATE_Write( tNetPacket );
	tNetPacket.w_u8		( (m_can_take)? 1 : 0 );
	tNetPacket.w_u8		( (m_closed)? 1 : 0 );
	tNetPacket.w_stringZ( m_tip_text );
}

void CSE_ALifeInventoryBox::UPDATE_Read( NET_Packet &tNetPacket )
{
	inherited::UPDATE_Read( tNetPacket );
}

void CSE_ALifeInventoryBox::UPDATE_Write( NET_Packet &tNetPacket )
{
	inherited::UPDATE_Write( tNetPacket );
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeInventoryBox::FillProps( LPCSTR pref, PropItemVec& values )
{
	inherited::FillProps( pref, values );
}
#endif // #ifndef XRGAME_EXPORTS
