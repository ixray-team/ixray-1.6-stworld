#include "stdafx.h"
#include "GameObject.h"
//#include "../Include/xrRender/RenderVisual.h"
#include "../Include/xrRender/RenderVisual.h"
#include "../xrphysics/PhysicsShell.h"
#include "physicobject.h"
#include "HangingLamp.h"
#include "../xrphysics/PhysicsShell.h"
#include "ph_shell_interface.h"
#include "xrserver_objects_alife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "game_cl_base.h"
#include "object_factory.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrEngine/igame_level.h"
#include "level.h"
#include "../xrphysics/MathUtils.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "game_cl_mp.h"
#include "reward_event_generator.h"
#include "../xrengine/xr_collide_form.h"
#include "animation_movement_controller.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

#ifdef DEBUG
#	include "debug_renderer.h"
#	include "PHDebug.h"
#endif

ENGINE_API bool g_dedicated_server;

CGameObject::CGameObject		()
{
	init						();
	//-----------------------------------------
	m_bCrPr_Activated			= false;
	m_dwCrPr_ActivationStep		= 0;
	m_spawn_time				= 0;
	m_anim_mov_ctrl				= 0;
}

CGameObject::~CGameObject		()
{
	VERIFY						( !animation_movement( ) );
//	VERIFY						(!m_ini_file);
	VERIFY						(!m_spawned);
}

void CGameObject::init			()
{
//	m_ini_file					= 0;
	m_spawned					= false;
}

void CGameObject::Load(LPCSTR section)
{
	inherited::Load			(section);
	ISpatial*		self				= smart_cast<ISpatial*> (this);
	if (self)	{
		self->spatial.type	|=	STYPE_VISIBLEFORAI;	
		self->spatial.type	&= ~STYPE_REACTTOSOUND;
	}
}

void CGameObject::reinit	()
{
	m_visual_callback.clear	();
}

void CGameObject::reload	(LPCSTR section)
{
}

void CGameObject::net_Destroy	()
{
	VERIFY					(m_spawned);
	if( m_anim_mov_ctrl )
					destroy_anim_mov_ctrl	();

//	xr_delete				(m_ini_file);

	if (Visual() && smart_cast<IKinematics*>(Visual()))
		smart_cast<IKinematics*>(Visual())->Callback	(0,0);

	inherited::net_Destroy						();
	setReady									(FALSE);
	
	g_pGameLevel->Objects.net_Unregister		(this);
	
	if (this == Level().CurrentActor())
	{
		Level().SetControlEntity			(0);
		Level().SetEntity					(0);	// do not switch !!!
	}

	Level().RemoveObject_From_4CrPr(this);

//.	Parent									= 0;

	m_spawned								= false;
}

void CGameObject::OnEvent		(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_HIT:
	case GE_HIT_STATISTIC:
		{
/*
			u16				id,weapon_id;
			Fvector			dir;
			float			power, impulse;
			s16				element;
			Fvector			position_in_bone_space;
			u16				hit_type;
			float			ap = 0.0f;

			P.r_u16			(id);
			P.r_u16			(weapon_id);
			P.r_dir			(dir);
			P.r_float		(power);
			P.r_s16			(element);
			P.r_vec3		(position_in_bone_space);
			P.r_float		(impulse);
			P.r_u16			(hit_type);	//hit type
			if ((ALife::EHitType)hit_type == ALife::eHitTypeFireWound)
			{
				P.r_float	(ap);
			}

			CObject*	Hitter = Level().Objects.net_Find(id);
			CObject*	Weapon = Level().Objects.net_Find(weapon_id);

			SHit	HDS = SHit(power, dir, Hitter, element, position_in_bone_space, impulse, (ALife::EHitType)hit_type, ap);
*/
			SHit	HDS;
			HDS.PACKET_TYPE = type;
			HDS.Read_Packet_Cont(P);
//			Msg("Hit received: %d[%d,%d]", HDS.whoID, HDS.weaponID, HDS.BulletID);
			CObject*	Hitter = Level().Objects.net_Find(HDS.whoID);
			CObject*	Weapon = Level().Objects.net_Find(HDS.weaponID);
			HDS.who		= Hitter;
			if (!HDS.who)
			{
				Msg("! ERROR: hitter object [%d] is NULL on client.", HDS.whoID);
			}
			//-------------------------------------------------------
			switch (HDS.PACKET_TYPE)
			{
			case GE_HIT_STATISTIC:
				{
					Game().m_WeaponUsageStatistic->OnBullet_Check_Request(&HDS);
				}break;
			default:
				{
				}break;
			}
			SetHitInfo(Hitter, Weapon, HDS.bone(), HDS.p_in_bone_space, HDS.dir);
			Hit				(&HDS);
			//---------------------------------------------------------------------------
			Game().m_WeaponUsageStatistic->OnBullet_Check_Result(false);
			game_cl_mp*	mp_game = smart_cast<game_cl_mp*>(&Game());
			if (mp_game->get_reward_generator())
				mp_game->get_reward_generator()->OnBullet_Hit(Hitter, this, Weapon, HDS.boneID);
			//---------------------------------------------------------------------------
		}
		break;
	case GE_DESTROY:
		{
			if ( H_Parent() )
			{
				Msg( "! ERROR (GameObject): GE_DESTROY arrived to object[%d][%s], that has parent[%d][%s], frame[%d]",
					ID(), cNameSect().c_str(),
					H_Parent()->ID(), H_Parent()->cName().c_str(), Device.dwFrame );
				
				// This object will be destroy on call function <H_Parent::Destroy>
				// or it will be call <H_Parent::Reject>  ==>  H_Parent = NULL
				// !!! ___ it is necessary to be check!
				break;
			}
#ifdef MP_LOGGING
//			Msg("--- Object: GE_DESTROY of [%d][%s]", ID(), cNameSect().c_str());
#endif // MP_LOGGING

			setDestroy		(TRUE);
//			MakeMeCrow		();
		}
		break;
	}
}

void VisualCallback(IKinematics *tpKinematics);

BOOL CGameObject::net_Spawn		(CSE_Abstract*	DC)
{
	VERIFY							(!m_spawned);
	m_spawned						= true;
	m_spawn_time					= Device.dwFrame;

	CSE_Abstract					*E = (CSE_Abstract*)DC;
	VERIFY							(E);

	const CSE_Visual				*visual	= smart_cast<const CSE_Visual*>(E);
	if (visual) {
		cNameVisual_set				(visual_name(E));
		if (visual->flags.test(CSE_Visual::flObstacle)) {
			ISpatial				*self = smart_cast<ISpatial*>(this);
			self->spatial.type		|=	STYPE_OBSTACLE;
		}
	}

	// Naming
	cName_set						(E->s_name);
	cNameSect_set					(E->s_name);
	if (E->name_replace()[0])
		cName_set					(E->name_replace());
	
	//R_ASSERT(Level().Objects.net_Find(E->ID) == NULL);
	if (Level().Objects.net_Find(E->ID) != NULL)
	{
		Msg("! ERROR: object [%d] already exists", E->ID);
		return FALSE;
	}


	setID							(E->ID);
	
	// XForm
	XFORM().setXYZ					(E->o_Angle);
	Position().set					(E->o_Position);
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Spawn obj %s Position set from CSE_Abstract %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif
	VERIFY							(_valid(renderable.xform));
	VERIFY							(!fis_zero(DET(renderable.xform)));
//	CSE_ALifeObject					*O = smart_cast<CSE_ALifeObject*>(E);

//	if (O && xr_strlen(O->m_ini_string)) {
//#pragma warning(push)
//#pragma warning(disable:4238)
//		m_ini_file					= xr_new<CInifile>(
//			&IReader				(
//				(void*)(*(O->m_ini_string)),
//				O->m_ini_string.size()
//			),
//			FS.get_path("$game_config$")->m_Path
//		);
//#pragma warning(pop)
//	}

	// Net params
	setLocal						(E->s_flags.is(M_SPAWN_OBJECT_LOCAL));
	setReady						(TRUE);
	g_pGameLevel->Objects.net_Register	(this);

	reload						(*cNameSect());
	
	reinit						();

	////load custom user data from server
	//if(!E->client_data.empty())
	//{	
	//	IReader			ireader = IReader(&*E->client_data.begin(), E->client_data.size());
	//	net_Load		(ireader);
	//}

	inherited::net_Spawn		(DC);

	m_bObjectRemoved			= false;

	return TRUE;
}


void CGameObject::spatial_move	()
{
	inherited::spatial_move			();
}

#ifdef DEBUG
void			CGameObject::dbg_DrawSkeleton	()
{
	CCF_Skeleton* Skeleton = smart_cast<CCF_Skeleton*>(collidable.model);
	if (!Skeleton) return;
	Skeleton->_dbg_refresh();

	const CCF_Skeleton::ElementVec& Elements = Skeleton->_GetElements();
	for (CCF_Skeleton::ElementVec::const_iterator I=Elements.begin(); I!=Elements.end(); I++){
		if (!I->valid())		continue;
		switch (I->type){
			case SBoneShape::stBox:{
				Fmatrix M;
				M.invert			(I->b_IM);
				Fvector h_size		= I->b_hsize;
				Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 255, 0, 255));
								   }break;
			case SBoneShape::stCylinder:{
				Fmatrix M;
				M.c.set				(I->c_cylinder.m_center);
				M.k.set				(I->c_cylinder.m_direction);
				Fvector				h_size;
				h_size.set			(I->c_cylinder.m_radius,I->c_cylinder.m_radius,I->c_cylinder.m_height*0.5f);
				Fvector::generate_orthonormal_basis(M.k,M.j,M.i);
				Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 127, 255, 255));
										}break;
			case SBoneShape::stSphere:{
				Fmatrix				l_ball;
				l_ball.scale		(I->s_sphere.R, I->s_sphere.R, I->s_sphere.R);
				l_ball.translate_add(I->s_sphere.P);
				Level().debug_renderer().draw_ellipse(l_ball, color_rgba(0, 255, 0, 255));
									  }break;
		};
	};	
}
#endif

void CGameObject::renderable_Render	()
{
	inherited::renderable_Render();
	::Render->set_Transform		(&XFORM());
	::Render->add_Visual		(Visual());
	Visual()->getVisData().hom_frame = Device.dwFrame;
}

CObject::SavedPosition CGameObject::ps_Element(u32 ID) const
{
	VERIFY(ID<ps_Size());
	inherited::SavedPosition	SP	=	PositionStack[ID];
	SP.dwTime					+=	Level().timeServer_Delta();
	return SP;
}

void CGameObject::u_EventGen(NET_Packet& P, u32 type, u32 dest)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		(u16(type&0xffff));
	P.w_u16		(u16(dest&0xffff));
}

void CGameObject::u_EventSend(NET_Packet& P, u32 dwFlags )
{
	Level().Send(P, dwFlags);
}

#include "bolt.h"
void CGameObject::OnH_B_Chield()
{
	inherited::OnH_B_Chield();
	///PHSetPushOut();????
}

void CGameObject::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);
}

void CGameObject::add_visual_callback		(visual_callback *callback)
{
	VERIFY						(smart_cast<IKinematics*>(Visual()));
	CALLBACK_VECTOR_IT			I = std::find(visual_callbacks().begin(),visual_callbacks().end(),callback);
	VERIFY						(I == visual_callbacks().end());

	if (m_visual_callback.empty())	SetKinematicsCallback(true);
//		smart_cast<IKinematics*>(Visual())->Callback(VisualCallback,this);
	m_visual_callback.push_back	(callback);
}

void CGameObject::remove_visual_callback	(visual_callback *callback)
{
	CALLBACK_VECTOR_IT			I = std::find(m_visual_callback.begin(),m_visual_callback.end(),callback);
	VERIFY						(I != m_visual_callback.end());
	m_visual_callback.erase		(I);
	if (m_visual_callback.empty())	SetKinematicsCallback(false);
//		smart_cast<IKinematics*>(Visual())->Callback(0,0);
}

void CGameObject::SetKinematicsCallback		(bool set)
{
	if(!Visual())	return;
	if (set)
		smart_cast<IKinematics*>(Visual())->Callback(VisualCallback,this);
	else
		smart_cast<IKinematics*>(Visual())->Callback(0,0);
};

void VisualCallback	(IKinematics *tpKinematics)
{
	CGameObject						*game_object = static_cast<CGameObject*>(static_cast<CObject*>(tpKinematics->GetUpdateCallbackParam()));
	VERIFY							(game_object);
	
	CGameObject::CALLBACK_VECTOR_IT	I = game_object->visual_callbacks().begin();
	CGameObject::CALLBACK_VECTOR_IT	E = game_object->visual_callbacks().end();
	for ( ; I != E; ++I)
		(*I)						(tpKinematics);
}

bool CGameObject::NeedToDestroyObject()	const
{
	return false;
}

void CGameObject::DestroyObject()			
{
	
	if(m_bObjectRemoved)	return;
	m_bObjectRemoved		= true;
	if (getDestroy())		return;

	if (Local())
	{	
		NET_Packet		P;
		u_EventGen		(P,GE_DESTROY,ID());
		u_EventSend		(P);
	}
}

void CGameObject::shedule_Update	(u32 dt)
{
	//уничтожить
	if(NeedToDestroyObject())
	{
#ifndef MASTER_GOLD
		Msg("--NeedToDestroyObject for [%d][%d]", ID(), Device.dwFrame);
#endif // #ifndef MASTER_GOLD
		DestroyObject			();
	}

	// Msg							("-SUB-:[%x][%s] CGameObject::shedule_Update",smart_cast<void*>(this),*cName());
	inherited::shedule_Update	(dt);
}

BOOL CGameObject::net_SaveRelevant	()
{
	return	TRUE;
}

//игровое имя объекта
LPCSTR CGameObject::Name () const
{
	return	(*cName());
}


void CGameObject::net_Relcase			(CObject* O)
{
	inherited::net_Relcase		(O);
}

LPCSTR CGameObject::visual_name		(CSE_Abstract *server_entity)
{
	const CSE_Visual			*visual	= smart_cast<const CSE_Visual*>(server_entity);
	VERIFY						(visual);
	return						(visual->get_visual());
}

bool		CGameObject::	animation_movement_controlled	( ) const	
{ 
	return	!!animation_movement() && animation_movement()->IsActive();
}

void CGameObject::update_animation_movement_controller	()
{
	if (!m_anim_mov_ctrl )
		return;

	if (m_anim_mov_ctrl->IsActive())
	{
		m_anim_mov_ctrl->OnFrame	();
		return;
	}

	destroy_anim_mov_ctrl		();
}

void	CGameObject::OnChangeVisual	( )
{
	inherited::OnChangeVisual( );
	if ( m_anim_mov_ctrl )
		destroy_anim_mov_ctrl( );
}

bool CGameObject::shedule_Needed( )
{
	return						(!getDestroy());
}

void CGameObject::create_anim_mov_ctrl	( CBlend *b, Fmatrix *start_pose, bool local_animation )
{
	if( animation_movement_controlled( ) )
	{
		m_anim_mov_ctrl->NewBlend	(
			b,
			start_pose ? *start_pose : XFORM(),
			local_animation
		);
	}
	else
	{
//		start_pose		= &renderable.xform;
		if( m_anim_mov_ctrl )
			destroy_anim_mov_ctrl();

		VERIFY2			(
			start_pose,
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);

		VERIFY2			(
			!animation_movement(),
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);
		
		VERIFY			(Visual());
		IKinematics		*K = Visual( )->dcast_PKinematics( );
		VERIFY			( K );

		m_anim_mov_ctrl	= xr_new<animation_movement_controller>( &XFORM(), *start_pose, K, b ); 
	}
}

void CGameObject::destroy_anim_mov_ctrl	()
{
	xr_delete			( m_anim_mov_ctrl );
}

IC	bool similar						(const Fmatrix &_0, const Fmatrix &_1, const float &epsilon = EPS)
{
	if (!_0.i.similar(_1.i,epsilon))
		return						(false);

	if (!_0.j.similar(_1.j,epsilon))
		return						(false);

	if (!_0.k.similar(_1.k,epsilon))
		return						(false);

	if (!_0.c.similar(_1.c,epsilon))
		return						(false);

	// note: we do not compare projection here
	return							(true);
}

void CGameObject::UpdateCL			()
{
	inherited::UpdateCL				();
	
	if (H_Parent())
		return;
}

#ifdef DEBUG

void render_box						(IRenderVisual *visual, const Fmatrix &xform, const Fvector &additional, bool draw_child_boxes, const u32 &color)
{
	CDebugRenderer			&renderer = Level().debug_renderer();
	IKinematics				*kinematics = smart_cast<IKinematics*>(visual);
	VERIFY					(kinematics);
	u16						bone_count = kinematics->LL_BoneCount();
	VERIFY					(bone_count);
	u16						visible_bone_count = kinematics->LL_VisibleBoneCount();
	if (!visible_bone_count)
		return;

	Fmatrix					matrix;
	Fvector					*points = (Fvector*)_alloca(visible_bone_count*8*sizeof(Fvector));
	Fvector					*I = points;
	for (u16 i=0; i<bone_count; ++i) {
		if (!kinematics->LL_GetBoneVisible(i))
			continue;
		
		const Fobb			&obb = kinematics->LL_GetData(i).obb;
		if (fis_zero(obb.m_halfsize.square_magnitude())) {
			VERIFY			(visible_bone_count > 1);
			--visible_bone_count;
			continue;
		}

		Fmatrix				Mbox;
		obb.xform_get		(Mbox);

		const Fmatrix		&Mbone = kinematics->LL_GetBoneInstance(i).mTransform;
		Fmatrix				X;
		matrix.mul_43		(xform,X.mul_43(Mbone,Mbox));

		Fvector				half_size = Fvector().add(obb.m_halfsize,additional);
		matrix.mulB_43		(Fmatrix().scale(half_size));

		if (draw_child_boxes)
			renderer.draw_obb	(matrix,color);

		static const Fvector	local_points[8] = {
			Fvector().set(-1.f,-1.f,-1.f),
			Fvector().set(-1.f,-1.f,+1.f),
			Fvector().set(-1.f,+1.f,+1.f),
			Fvector().set(-1.f,+1.f,-1.f),
			Fvector().set(+1.f,+1.f,+1.f),
			Fvector().set(+1.f,+1.f,-1.f),
			Fvector().set(+1.f,-1.f,+1.f),
			Fvector().set(+1.f,-1.f,-1.f)
		};
		
		for (u32 i=0; i<8; ++i, ++I)
			matrix.transform_tiny	(*I,local_points[i]);
	}

	VERIFY						(visible_bone_count);
	if (visible_bone_count == 1) {
		renderer.draw_obb		(matrix,color);
		return;
	}
}

void CGameObject::OnRender			()
{
}
#endif // DEBUG