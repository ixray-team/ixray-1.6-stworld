#include "stdafx.h"
#include "WeaponBinoculars.h"

#include "xr_level_controller.h"

#include "level.h"
#include "ui\UIFrameWindow.h"
#include "inventory.h"

CWeaponBinoculars::CWeaponBinoculars()
{
	m_bVision		= false;
}

CWeaponBinoculars::~CWeaponBinoculars()
{
}

void CWeaponBinoculars::Load	(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	m_sounds.LoadSound(section, "snd_zoomin",  "sndZoomIn",		false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section, "snd_zoomout", "sndZoomOut",	false, SOUND_TYPE_ITEM_USING);
	m_bVision = !!pSettings->r_bool(section,"vision_present");
}


bool CWeaponBinoculars::Action(u16 cmd, u32 flags) 
{
	switch(cmd) 
	{
	case kWPN_FIRE : 
		return inherited::Action(kWPN_ZOOM, flags);
	}

	return inherited::Action(cmd, flags);
}

void CWeaponBinoculars::OnZoomIn		()
{
	if(H_Parent() && !IsZoomed())
	{
		m_sounds.StopSound("sndZoomOut");
		bool b_hud_mode = (Level().CurrentActor() == H_Parent());
		m_sounds.PlaySound("sndZoomIn", H_Parent()->Position(), H_Parent(), b_hud_mode);
	}
	inherited::OnZoomIn		();
}

void CWeaponBinoculars::OnZoomOut		()
{
	if(H_Parent() && IsZoomed() && !IsRotatingToZoom())
	{
		m_sounds.StopSound("sndZoomIn");
		bool b_hud_mode = (Level().CurrentActor() == H_Parent());	
		m_sounds.PlaySound("sndZoomOut", H_Parent()->Position(), H_Parent(), b_hud_mode);
	}


	inherited::OnZoomOut();
}

BOOL CWeaponBinoculars::net_Spawn(CSE_Abstract* DC)
{
	inherited::net_Spawn	(DC);
	return					TRUE;
}

void	CWeaponBinoculars::net_Destroy()
{
	inherited::net_Destroy();
}

void	CWeaponBinoculars::UpdateCL()
{
	inherited::UpdateCL();
}

bool CWeaponBinoculars::render_item_ui_query()
{
	return false;
}

void CWeaponBinoculars::render_item_ui()
{
	inherited::render_item_ui	();
}

void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
	float def_fov = float(g_fov);
	float min_zoom_k = 0.3f;
	float zoom_step_count = 3.0f;
	float delta_factor_total = def_fov-scope_factor;
	VERIFY(delta_factor_total>0);
	min_zoom_factor = def_fov-delta_factor_total*min_zoom_k;
	delta = (delta_factor_total*(1-min_zoom_k) )/zoom_step_count;

}

void CWeaponBinoculars::ZoomInc()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

	float f					= GetZoomFactor()-delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );
}

void CWeaponBinoculars::ZoomDec()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor,delta,min_zoom_factor);

	float f					= GetZoomFactor()+delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );

}
void CWeaponBinoculars::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data		(m_fRTZoomFactor,output_packet);
}

void CWeaponBinoculars::load(IReader &input_packet)
{
	inherited::load(input_packet);
	load_data		(m_fRTZoomFactor,input_packet);
}

bool CWeaponBinoculars::GetBriefInfo( II_BriefInfo& info )
{
	info.clear();
	info.name = NameShort();
	info.icon = cNameSect();
	return true;
}

void CWeaponBinoculars::net_Relcase	(CObject *object)
{
}

bool CWeaponBinoculars::can_kill	() const
{
	return			(false);
}