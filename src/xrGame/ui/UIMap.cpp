#include "stdafx.h"
#include "../level.h"
#include "../map_location.h"
#include "../map_manager.h"
#include "../map_spot.h"
#include "UIMap.h"
#include "../../xrEngine/xr_input.h"		//remove me !!!

const u32			activeLocalMapColor			= 0xffffffff;//0xffc80000;
const u32			inactiveLocalMapColor		= 0xffffffff;//0xff438cd1;
const u32			ourLevelMapColor			= 0xffffffff;


CUICustomMap::CUICustomMap ()
{
	m_BoundRect_.set		(0,0,0,0);
	SetWindowName			("map");
	m_flags.zero			();
	SetPointerDistance		(0.0f);
}

void CUICustomMap::Initialize(shared_str name, LPCSTR sh_name)
{
	CInifile* levelIni	= NULL;
	if(name==g_pGameLevel->map_name())
		levelIni = g_pGameLevel->pLevel;
	else
	{
		string_path					map_cfg_fn;
		string_path					fname;
		strconcat					(sizeof(fname),fname,name.c_str(), "\\level.ltx");
		FS.update_path				(map_cfg_fn, "$game_levels$", fname);
		levelIni					= xr_new<CInifile>(map_cfg_fn);
	}

	if(levelIni->section_exist("level_map"))
	{
		Init_internal	(name, *levelIni, "level_map", sh_name);
	}else
	{
		Msg("! default LevelMap used for level[%s]",name.c_str());
		Init_internal	(name, *pGameIni, "def_map", sh_name);
		m_name			= name;
	}
	if(levelIni != g_pGameLevel->pLevel)
		xr_delete(levelIni);
}

CUICustomMap::~CUICustomMap()
{}

void CUICustomMap::Update()
{
	SetPointerDistance		(0.0f);
	if(!Locked())
		UpdateSpots			();

	CUIStatic::Update		();
}

void CUICustomMap::Draw()
{
	UI().PushScissor		(WorkingArea());
	CUIStatic::Draw			();
	UI().PopScissor			();
}


void CUICustomMap::Init_internal(const shared_str& name, CInifile& pLtx, const shared_str& sect_name, LPCSTR sh_name)
{
	m_name					= name;
	Fvector4				tmp;

	m_texture				= pLtx.r_string(sect_name,"texture");
	m_shader_name			= sh_name;
	tmp						= pLtx.r_fvector4(sect_name,"bound_rect");
	
	if(!Heading())
	{
		tmp.x					*= UI().get_current_kx();
		tmp.z					*= UI().get_current_kx();
	}

	m_BoundRect_.set		(tmp.x, tmp.y, tmp.z, tmp.w);

	Fvector2 sz;
	m_BoundRect_.getsize	(sz);
	CUIStatic::SetWndSize	(sz);
	CUIStatic::SetWndPos	(Fvector2().set(0,0));
	CUIStatic::InitTextureEx(m_texture.c_str(), m_shader_name.c_str());
	
	SetStretchTexture		(true);
}

void rotation_(float x, float y, const float angle, float& x_, float& y_, float kx)
{
	float _sc 		= _cos(angle);
	float _sn 		= _sin(angle);
	x_				= x*_sc+y*_sn;
	y_				= y*_sc-x*_sn;
	x_				*= kx;
}

Fvector2 CUICustomMap::ConvertLocalToReal(const Fvector2& src, Frect const& bound_rect)
{
	Fvector2 res; 
	res.x = bound_rect.lt.x + src.x/GetCurrentZoom().x;
	res.y = bound_rect.height() + bound_rect.lt.y - src.y/GetCurrentZoom().x;

	return res;
}

Fvector2 CUICustomMap::ConvertRealToLocal  (const Fvector2& src, bool for_drawing)// meters->pixels (relatively own left-top pos)
{
	Fvector2 res;
	if( !Heading() )
	{
		Frect bound_rect	= BoundRect();
		bound_rect.x1		/= UI().get_current_kx();
		bound_rect.x2		/= UI().get_current_kx();
		res					= ConvertRealToLocalNoTransform(src, bound_rect);
		res.x				*= UI().get_current_kx();
	}else
	{
		Fvector2 heading_pivot = GetStaticItem()->GetHeadingPivot();
	
		res						= ConvertRealToLocalNoTransform(src, BoundRect());
		res.sub					(heading_pivot);
		rotation_				(res.x, res.y, GetHeading(), res.x, res.y, for_drawing?UI().get_current_kx():1.0f);
		
		res.add					(heading_pivot);
	};
	return res;
}

Fvector2 CUICustomMap::ConvertRealToLocalNoTransform  (const Fvector2& src, Frect const& bound_rect)// meters->pixels (relatively own left-top pos)
{
	Fvector2 res;
	res.x = (src.x-bound_rect.lt.x) * GetCurrentZoom().x;
	res.y = (bound_rect.height()-(src.y-bound_rect.lt.y)) * GetCurrentZoom().x;

	return res;
}

//position and heading for drawing pointer to src pos
bool CUICustomMap::GetPointerTo(const Fvector2& src, float item_radius, Fvector2& pos, float& heading)
{
	Frect		clip_rect_abs			= WorkingArea(); //absolute rect coords
	Frect		map_rect_abs;
	GetAbsoluteRect(map_rect_abs);

	Frect		rect;
	BOOL res = rect.intersection(clip_rect_abs, map_rect_abs);
	if(!res) return false;
	
	rect = clip_rect_abs;
	rect.sub(map_rect_abs.lt.x,map_rect_abs.lt.y);

	Fbox2 f_clip_rect_local;
	f_clip_rect_local.set(rect.x1, rect.y1, rect.x2, rect.y2 );

	Fvector2 f_center;
	f_clip_rect_local.getcenter(f_center);

	Fvector2 f_dir, f_src;

	f_src.set(src.x, src.y );
	f_dir.sub(f_center, f_src );
	f_dir.normalize_safe();
	Fvector2 f_intersect_point;
	res = f_clip_rect_local.Pick2(f_src,f_dir,f_intersect_point);
	if(!res)
		return false;


	heading = -f_dir.getH();

	f_intersect_point.mad(f_intersect_point,f_dir,item_radius );

	pos.set( iFloor(f_intersect_point.x), iFloor(f_intersect_point.y) );
	return true;
}


void CUICustomMap::FitToWidth(float width)
{
	float k			= m_BoundRect_.width()/m_BoundRect_.height();
	float w			= width;
	float h			= width/k;
	SetWndRect		(Frect().set(0.0f,0.0f,w,h));
}

void CUICustomMap::FitToHeight(float height)
{
	float k			= m_BoundRect_.width()/m_BoundRect_.height();
	float h			= height;
	float w			= k*height;

	SetWndRect		(Frect().set(0.0f,0.0f,w,h));
}


void CUICustomMap::OptimalFit(const Frect& r)
{
	if ((BoundRect().height()/r.height())<(BoundRect().width()/r.width()))
		FitToHeight	(r.height());
	else
		FitToWidth	(r.width());
}

// try to positioning clipRect center to vNewPoint
void CUICustomMap::SetActivePoint(const Fvector &vNewPoint)
{
	Fvector2 pos;
	pos.set(vNewPoint.x,vNewPoint.z);
	Frect bound = BoundRect();
	if( FALSE==bound.in(pos) )return;

	Fvector2	pos_on_map		= ConvertRealToLocalNoTransform(pos, BoundRect());
	Frect		map_abs_rect;
	GetAbsoluteRect(map_abs_rect);
	Fvector2	pos_abs;

	pos_abs.set(map_abs_rect.lt);
	pos_abs.add(pos_on_map);

	Fvector2					clip_center;
	WorkingArea().getcenter		(clip_center);
	clip_center.sub				(pos_abs);
	MoveWndDelta				(clip_center);
	SetHeadingPivot				(pos_on_map, Fvector2().set(0,0), false);
}

bool CUICustomMap::IsRectVisible(Frect r)
{
	Fvector2 pos;
	GetAbsolutePos(pos);
	r.add(pos.x,pos.y);

	return !!WorkingArea().intersected(r);
}

bool CUICustomMap::NeedShowPointer(Frect r)
{
	Frect map_visible_rect = WorkingArea();
	map_visible_rect.shrink(5,5);
	Fvector2 pos;
	GetAbsolutePos(pos);
	r.add(pos.x,pos.y);

	return !map_visible_rect.intersected(r);
}

void	CUICustomMap::SendMessage			(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

CUIMiniMap::CUIMiniMap()
{}

CUIMiniMap::~CUIMiniMap()
{}

void CUIMiniMap::Init_internal(const shared_str& name, CInifile& pLtx, const shared_str& sect_name, LPCSTR sh_name)
{
	inherited::Init_internal	(name, pLtx, sect_name, sh_name);
	CUIStatic::SetTextureColor	(0x7fffffff);
}

void CUIMiniMap::UpdateSpots()
{
	DetachAll					();
	Locations& ls = Level().MapManager().Locations();
	for(Locations_it it=ls.begin(); it!=ls.end(); ++it)
		(*it).location->UpdateMiniMap(this);
}

void  CUIMiniMap::Draw()
{
	u32	segments_count			= 20;

	UIRender->SetShader			(*m_UIStaticItem.GetShader());
	UIRender->StartPrimitive	(segments_count*3, IUIRender::ptTriList, UI().m_currentPointType);

	u32 color					= m_UIStaticItem.GetTextureColor();
	float angle					= GetHeading();



	float kx =	UI().get_current_kx();

	// clip poly
	sPoly2D					S;
	S.resize				(segments_count);
	float segment_ang		= PI_MUL_2/segments_count;
	float pt_radius			= WorkingArea().width()/2.0f;
	Fvector2				center;
	WorkingArea().getcenter	(center);

	float tt_radius			= pt_radius/GetWidth();
	float k_tt_height		= GetWidth()/GetHeight();
							
	Fvector2 tt_offset;
	tt_offset.set			(m_UIStaticItem.vHeadingPivot);
	tt_offset.x				/= GetWidth();
	tt_offset.y				/= GetHeight();
	
	Fvector2				m_scale_;	
	m_scale_.set			( float(Device.dwWidth)/UI_BASE_WIDTH, float(Device.dwHeight)/UI_BASE_HEIGHT );
	
	for(u32 idx=0; idx<segments_count;++idx)
	{
		float cosPT			= _cos(segment_ang*idx + angle);
		float sinPT			= _sin(segment_ang*idx + angle);

		float cosTX			= _cos(segment_ang*idx);
		float sinTX			= _sin(segment_ang*idx);

		S[idx].pt.set		(pt_radius*cosPT*kx,		-pt_radius*sinPT);
		S[idx].uv.set		(tt_radius*cosTX,			-tt_radius*sinTX*k_tt_height);
		S[idx].uv.add		(tt_offset);
		S[idx].pt.add		(center);

		S[idx].pt.x			*= m_scale_.x;
		S[idx].pt.y			*= m_scale_.y;
	}

	for (u32 idx=0; idx<segments_count-2; ++idx)
	{
		UIRender->PushPoint(S[0+0].pt.x,	S[0+0].pt.y,	0, color, S[0+0].uv.x,		S[0+0].uv.y);
		UIRender->PushPoint(S[idx+2].pt.x,	S[idx+2].pt.y,	0, color, S[idx+2].uv.x,	S[idx+2].uv.y);
		UIRender->PushPoint(S[idx+1].pt.x,	S[idx+1].pt.y,	0, color, S[idx+1].uv.x,	S[idx+1].uv.y);
	}

	UIRender->FlushPrimitive	();


//------------
	CUIWindow::Draw(); //draw childs
}

bool CUIMiniMap::GetPointerTo(const Fvector2& src, float item_radius, Fvector2& pos, float& heading)
{
	Fvector2 clip_center = GetStaticItem()->GetHeadingPivot();
	float map_radius	= WorkingArea().width()/2.0f;
	Fvector2			direction;

	direction.sub		(clip_center, src);
	heading				= -direction.getH();

	float kx			= UI().get_current_kx();
	float cosPT			= _cos(heading);
	float sinPT			= _sin(heading);
	pos.set				(-map_radius*sinPT*kx,		-map_radius*cosPT);
	pos.add				(clip_center);

	return				true;
}

bool CUIMiniMap::NeedShowPointer(Frect r)
{
	Fvector2 clip_center = GetStaticItem()->GetHeadingPivot();

	Fvector2			spot_pos;
	r.getcenter			(spot_pos);
	float dist			=clip_center.distance_to(spot_pos);
	float spot_radius	= r.width() / 2.0f;
	return				(dist+spot_radius > WorkingArea().width()/2.0f);
}

bool CUIMiniMap::IsRectVisible(Frect r)
{
	Fvector2 clip_center	= GetStaticItem()->GetHeadingPivot();
	float vis_radius		= WorkingArea().width() / 2.0f;
	Fvector2				rect_center;
	r.getcenter				(rect_center);
	float spot_radius		= r.width() / 2.0f;
	return clip_center.distance_to(rect_center)+spot_radius < vis_radius; //assume that all minimap spots are circular
}
