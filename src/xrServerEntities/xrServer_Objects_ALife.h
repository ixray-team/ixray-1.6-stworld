////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALifeH
#define xrServer_Objects_ALifeH

#include "xrServer_Objects.h"
#include "alife_space.h"
#include "game_graph_space.h"

#pragma warning(push)
#pragma warning(disable:4005)

#ifdef XRGAME_EXPORTS
	class 	CALifeSimulator;
#endif

class CSE_ALifeItemWeapon;

struct  SFillPropData
{
	RTokenVec 						locations[4];
	RStringVec						level_ids;
	RTokenVec 						story_names;
	RTokenVec 						spawn_story_names;
	RStringVec						character_profiles;
	RStringVec						smart_covers;
	xr_map<shared_str, u32>			location_colors;
	u32								counter;
	SFillPropData					();
	~SFillPropData					();
	void							load					();
	void							unload					();
	void							inc						();
	void							dec						();
};

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeDynamicObjectVisual,CSE_Abstract,CSE_Visual)
									CSE_ALifeDynamicObjectVisual(LPCSTR caSection);
	virtual							~CSE_ALifeDynamicObjectVisual();
	virtual CSE_Visual* __stdcall	visual					();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifePHSkeletonObject,CSE_ALifeDynamicObjectVisual,CSE_PHSkeleton)
									CSE_ALifePHSkeletonObject(LPCSTR caSection);
	virtual							~CSE_ALifePHSkeletonObject();
	virtual bool					can_save				() const;
	virtual CSE_Abstract			*cast_abstract			() {return this;}
public:
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeSpaceRestrictor,CSE_Abstract,CSE_Shape)
	u8								m_space_restrictor_type;

									CSE_ALifeSpaceRestrictor	(LPCSTR caSection);
	virtual							~CSE_ALifeSpaceRestrictor	();
	virtual ISE_Shape*  __stdcall	shape						();
SERVER_ENTITY_DECLARE_END


SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeObjectPhysic,CSE_ALifeDynamicObjectVisual,CSE_PHSkeleton)
	u32 							type;
	f32 							mass;
    shared_str 						fixed_bones;
									CSE_ALifeObjectPhysic	(LPCSTR caSection);
    virtual 						~CSE_ALifeObjectPhysic	();
	virtual bool					can_save				() const;
	virtual CSE_Abstract			*cast_abstract			() {return this;}
private:
					u32			m_freeze_time;
	static const	u32			m_freeze_delta_time;
#ifdef DEBUG	//only for testing interpolation
					u32			m_last_update_time;
	static const	u32			m_update_delta_time;
#endif
	static const	u32			random_limit;
					CRandom		m_relevent_random;

public:
	enum {
		inventory_item_state_enabled	= u8(1) << 0,
		inventory_item_angular_null		= u8(1) << 1,
		inventory_item_linear_null		= u8(1) << 2//,
	};
	union mask_num_items {
		struct {
			u8	num_items : 5;
			u8	mask      : 3;
		};
		u8		common;
	};
	/////////// network ///////////////
	u8								m_u8NumItems;
	bool							prev_freezed;
	bool							freezed;
	SPHNetState						State;
	
	virtual BOOL					Net_Relevant			();

SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeObjectHangingLamp,CSE_ALifeDynamicObjectVisual,CSE_PHSkeleton)

    void __stdcall 					OnChangeFlag	(PropValue* sender);
    enum{
        flPhysic					= (1<<0),
		flCastShadow				= (1<<1),
		flR1						= (1<<2),
		flR2						= (1<<3),
		flTypeSpot					= (1<<4),
        flPointAmbient				= (1<<5),
		flVolumetric				= (1<<6),
    };

    Flags16							flags;
// light color    
    u32								color;
    float							brightness;
    shared_str						color_animator;
// light texture    
	shared_str						light_texture;
// range
    float							range;
	float							m_virtual_size;
// bones&motions
	shared_str						light_ambient_bone;
	shared_str						light_main_bone;
    shared_str						fixed_bones;
// spot
	float							spot_cone_angle;
// ambient    
    float							m_ambient_radius;
    float							m_ambient_power;
	shared_str						m_ambient_texture;
//	volumetric
	float							m_volumetric_quality;
	float							m_volumetric_intensity;
	float							m_volumetric_distance;
// glow    
	shared_str						glow_texture;
	float							glow_radius;
// game
    float							m_health;
	
                                    CSE_ALifeObjectHangingLamp	(LPCSTR caSection);
    virtual							~CSE_ALifeObjectHangingLamp	();
	virtual bool					match_configuration			() const;
	virtual bool		__stdcall	validate					();
#ifndef XRGAME_EXPORTS
	virtual void 		__stdcall	on_render					(CDUInterface* du, ISE_AbstractLEOwner* owner, bool bSelected, const Fmatrix& parent,int priority, bool strictB2F);
#endif // #ifndef XRGAME_EXPORTS
	virtual CSE_Abstract			*cast_abstract				() {return this;}
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeObjectProjector,CSE_ALifeDynamicObjectVisual)
									CSE_ALifeObjectProjector	(LPCSTR caSection);
	virtual							~CSE_ALifeObjectProjector	();
SERVER_ENTITY_DECLARE_END


SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeObjectBreakable,CSE_ALifeDynamicObjectVisual)
    float							m_health;
									CSE_ALifeObjectBreakable	(LPCSTR caSection);
	virtual							~CSE_ALifeObjectBreakable	();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeObjectClimable,CSE_Abstract,CSE_Shape)
CSE_ALifeObjectClimable	(LPCSTR caSection);
shared_str						material;
virtual							~CSE_ALifeObjectClimable	();
virtual ISE_Shape*  __stdcall	shape				();

#ifndef XRGAME_EXPORTS
virtual	void		__stdcall	set_additional_info	(void* info);
#endif

SERVER_ENTITY_DECLARE_END



SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeTeamBaseZone,CSE_ALifeSpaceRestrictor)
									CSE_ALifeTeamBaseZone	(LPCSTR caSection);
	virtual							~CSE_ALifeTeamBaseZone	();

	u8								m_team;
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeInventoryBox,CSE_ALifeDynamicObjectVisual)
	bool				m_can_take;
	bool				m_closed;
	shared_str			m_tip_text;

						CSE_ALifeInventoryBox	(LPCSTR caSection);
	virtual				~CSE_ALifeInventoryBox	();
SERVER_ENTITY_DECLARE_END

#pragma warning(pop)

#endif