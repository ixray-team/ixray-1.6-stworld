#include "stdafx.h"
#include "map_location.h"
#include "map_spot.h"
#include "map_manager.h"

#include "level.h"
#include "../xrEngine/xr_object.h"
#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ui/UIXmlInit.h"
#include "ui/UIMap.h"
#include "actor.h"
#include "InventoryOwner.h"
#include "string_table.h"
#include "ActorHelmet.h"
#include "Inventory.h"

CMapLocation::CMapLocation(LPCSTR type, u16 object_id)
{
	m_flags.zero			();

	m_level_spot			= NULL;
	m_level_spot_pointer	= NULL;
	m_minimap_spot			= NULL;
	m_minimap_spot_pointer	= NULL;
	m_complex_spot			= NULL;
	m_complex_spot_pointer	= NULL;

	m_level_map_spot_border	= NULL;
	m_mini_map_spot_border	= NULL;
	m_complex_spot_border	= NULL;

	m_level_map_spot_border_na = NULL;
	m_mini_map_spot_border_na = NULL;
	m_complex_spot_border_na = NULL;

	m_objectID				= object_id;
	m_actual_time			= 0;
	m_owner_se_object		= NULL;
	m_flags.set				(eHintEnabled, TRUE);
	LoadSpot				(type, false);
	
	DisablePointer			();

	EnableSpot				();
	m_cached.m_Position.set	(10000,10000);
	m_cached.m_updatedFrame = u32(-1);
	m_cached.m_LevelName	= Level().map_name();
}

CMapLocation::~CMapLocation()
{
}

void CMapLocation::destroy()
{
	delete_data(m_level_spot);
	delete_data(m_level_spot_pointer);
	delete_data(m_minimap_spot);
	delete_data(m_minimap_spot_pointer);
	delete_data(m_complex_spot);
	delete_data(m_complex_spot_pointer);

	delete_data(m_level_map_spot_border);
	delete_data(m_mini_map_spot_border);
	delete_data(m_complex_spot_border);
	
	delete_data(m_level_map_spot_border_na);
	delete_data(m_mini_map_spot_border_na);
	delete_data(m_complex_spot_border_na);
}

CUIXml*	g_uiSpotXml=NULL;
void CMapLocation::LoadSpot(LPCSTR type, bool bReload)
{
	if ( !g_uiSpotXml )
	{
		g_uiSpotXml				= xr_new<CUIXml>();
		g_uiSpotXml->Load		(CONFIG_PATH, UI_PATH, "map_spots.xml");
	}

	XML_NODE* node = NULL;
	string512 path_base, path;
	xr_strcpy		(path_base,type);
	R_ASSERT3		(g_uiSpotXml->NavigateToNode(path_base,0), "XML node not found in file map_spots.xml", path_base);
	LPCSTR s		= g_uiSpotXml->ReadAttrib(path_base, 0, "hint", "no hint");
	SetHint			(s);
	
	s = g_uiSpotXml->ReadAttrib(path_base, 0, "store", NULL);
	if ( s )
	{
		m_flags.set( eSerailizable, TRUE);
	}

	s = g_uiSpotXml->ReadAttrib(path_base, 0, "no_offline", NULL);
	if ( s )
	{
		m_flags.set( eHideInOffline, TRUE);
	}

	m_ttl = g_uiSpotXml->ReadAttribInt(path_base, 0, "ttl", 0);
	if ( m_ttl > 0 )
	{
		m_flags.set( eTTL, TRUE);
		m_actual_time = Device.dwTimeGlobal+m_ttl*1000;
	}

	s = g_uiSpotXml->ReadAttrib(path_base, 0, "pos_to_actor", NULL);
	if ( s )
	{
		m_flags.set( ePosToActor, TRUE);
	}
	
	strconcat(sizeof(path),path,path_base,":level_map");
	node = g_uiSpotXml->NavigateToNode(path,0);
	if ( node )
	{
		LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_level_spot = xr_new<CMapSpot>(this);
			}
			m_level_spot->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload&&m_level_spot) );
		}

		m_spot_border_names[0] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "level_map_spot_border");
		m_spot_border_names[1] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

		str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_level_spot_pointer = xr_new<CMapSpotPointer>(this);
			}
			m_level_spot_pointer->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload && m_level_spot_pointer) );
		}
	}

	strconcat(sizeof(path),path,path_base,":mini_map");
	node = g_uiSpotXml->NavigateToNode(path,0);
	if ( node )
	{
		LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_minimap_spot = xr_new<CMiniMapSpot>(this);
			}
			m_minimap_spot->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload && m_minimap_spot) );
		}
		m_spot_border_names[2] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "mini_map_spot_border");
		m_spot_border_names[3] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

		str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_minimap_spot_pointer = xr_new<CMapSpotPointer>(this);
			}
			m_minimap_spot_pointer->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload && m_minimap_spot_pointer) );
		}
	}

	strconcat( sizeof(path), path, path_base, ":complex_spot" );
	node = g_uiSpotXml->NavigateToNode(path, 0);
	if ( node )
	{
		LPCSTR str = g_uiSpotXml->ReadAttrib(path, 0, "spot", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_complex_spot = xr_new<CComplexMapSpot>(this);
			}
			m_complex_spot->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload && m_complex_spot) );
		}
		m_spot_border_names[4] = g_uiSpotXml->ReadAttrib(path, 0, "spot_a", "complex_map_spot_border");
		m_spot_border_names[5] = g_uiSpotXml->ReadAttrib(path, 0, "spot_na", "");

		str = g_uiSpotXml->ReadAttrib(path, 0, "pointer", "");
		if( xr_strlen(str) )
		{
			if ( !bReload )
			{
				m_complex_spot_pointer = xr_new<CMapSpotPointer>(this);
			}
			m_complex_spot_pointer->Load(g_uiSpotXml,str);
		}else{
			VERIFY( !(bReload && m_complex_spot_pointer) );
		}
	}

	if ( m_minimap_spot == NULL && m_level_spot == NULL && m_complex_spot == NULL )
	{
		DisableSpot();
	}
}

void CMapLocation::CalcPosition()
{
	if(m_flags.test( ePosToActor) && Level().CurrentActor())
	{
		m_position_global		= Level().CurrentActor()->Position();
		m_cached.m_Position.set	(m_position_global.x, m_position_global.z);
		return;
	}

	CObject* pObject =  Level().Objects.net_Find(m_objectID);
	if(pObject)
	{
		m_position_global			= pObject->Position();
		m_cached.m_Position.set		(m_position_global.x, m_position_global.z);
	}
}

const Fvector2& CMapLocation::CalcDirection()
{
	if(Level().CurrentViewActor()&&Level().CurrentViewActor()->ID()==m_objectID )
	{
		m_cached.m_Direction.set(Device.vCameraDirection.x,Device.vCameraDirection.z);
	}else
	{
		CObject* pObject =  Level().Objects.net_Find(m_objectID);
		if(!pObject)
			m_cached.m_Direction.set(0.0f, 0.0f);
		else{
			const Fvector& op = pObject->Direction();
			m_cached.m_Direction.set(op.x, op.z);
		}
	}

	if(m_flags.test(ePosToActor)){
		CObject* pObject =  Level().Objects.net_Find(m_objectID);
		if(pObject){
			Fvector2 dcp,obj_pos;
			dcp.set(Device.vCameraPosition.x, Device.vCameraPosition.z);
			obj_pos.set(pObject->Position().x, pObject->Position().z);
			m_cached.m_Direction.sub(obj_pos, dcp);
			m_cached.m_Direction.normalize_safe();
		}
	}
	return					m_cached.m_Direction;
}

void CMapLocation::CalcLevelName()
{
	m_cached.m_LevelName = Level().map_name();
}

bool CMapLocation::Update() //returns actual
{
	R_ASSERT(m_cached.m_updatedFrame!=Device.dwFrame);
		

	if(	m_flags.test(eTTL) )
	{
		if( m_actual_time < Device.dwTimeGlobal)
		{
			m_cached.m_Actuality		= false;
			m_cached.m_updatedFrame		= Device.dwFrame;
			return						m_cached.m_Actuality;
		}
	}

	CObject* pObject					= Level().Objects.net_Find(m_objectID);
	
	if (m_owner_se_object || pObject )
	{
		m_cached.m_Actuality			= true;
		CalcPosition					();
	}else
		m_cached.m_Actuality			= false;

	m_cached.m_updatedFrame				= Device.dwFrame;
	return								m_cached.m_Actuality;
}

//xr_vector<u32> map_point_path;

class CMapSpot;

void CMapLocation::UpdateSpot(CUICustomMap* map, CMapSpot* sp )
{
	if( map->MapName() == GetLevelName() )
	{
		bool b_alife = false;

		//update spot position
		Fvector2 position	= GetPosition();

		m_position_on_map	= map->ConvertRealToLocal(position, (map->Heading())?false:true); //for visibility calculating

		sp->SetWndPos		(m_position_on_map);

		Frect wnd_rect		= sp->GetWndRect();

		if ( map->IsRectVisible(wnd_rect) ) 
		{
			//update heading if needed
			if( sp->Heading() && !sp->GetConstHeading() )
			{
				Fvector2 dir_global = CalcDirection();
				float h = dir_global.getH();
				float h_ = map->GetHeading()+h;
				sp->SetHeading( h_ );
			}
			map->AttachChild	(sp);
		}

		bool b_pointer =( GetSpotPointer(sp) && map->NeedShowPointer(wnd_rect));

		if(map->Heading())
		{
			m_position_on_map	= map->ConvertRealToLocal(position, true); //for drawing
			sp->SetWndPos		(m_position_on_map);
		}

		if(b_pointer)
			UpdateSpotPointer( map, GetSpotPointer(sp) );
	}
	else if ( Level().map_name() == map->MapName() && GetSpotPointer(sp) )
	{
		//GameGraph::_GRAPH_ID		dest_graph_id;

		//dest_graph_id		= m_owner_se_object->m_tGraphID;

		//map_point_path.clear();

		//VERIFY( Actor() );
		//GraphEngineSpace::CGameVertexParams		params(Actor()->locations().vertex_types(),flt_max);
		//bool res = ai().graph_engine().search(
		//	ai().game_graph(),
		//	u32(-1),
		//	dest_graph_id,
		//	&map_point_path,
		//	params
		//	);

		//if ( res )
		//{
		//	xr_vector<u32>::reverse_iterator it = map_point_path.rbegin();
		//	xr_vector<u32>::reverse_iterator it_e = map_point_path.rend();

		//	xr_vector<CLevelChanger*>::iterator lit = g_lchangers.begin();
		//	//xr_vector<CLevelChanger*>::iterator lit_e = g_lchangers.end();
		//	bool bDone						= false;
		//	//for(; (it!=it_e)&&(!bDone) ;++it){
		//	//	for(lit=g_lchangers.begin();lit!=lit_e; ++lit){

		//	//		if((*it)==u32(-1) )
		//	//		{
		//	//			bDone = true;
		//	//			break;
		//	//		}

		//	//	}
		//	//}
		//	static bool bbb = false;
		//	if(!bDone&&bbb)
		//	{
		//		Msg("! Error. Path from actor to selected map spot does not contain level changer :(");
		//		Msg("Path:");
		//		xr_vector<u32>::iterator it			= map_point_path.begin();
		//		xr_vector<u32>::iterator it_e		= map_point_path.end();
		//		for(; it!=it_e;++it){
		//			//					Msg("%d-%s",(*it),ai().game_graph().vertex(*it));
		//			Msg("[%d] level[%s]",(*it),*ai().game_graph().header().level(ai().game_graph().vertex(*it)->level_id()).name());
		//		}
		//		Msg("- Available LevelChangers:");
		//		xr_vector<CLevelChanger*>::iterator lit,lit_e;
		//		lit_e							= g_lchangers.end();
		//		for(lit=g_lchangers.begin();lit!=lit_e; ++lit){
		//			GameGraph::_GRAPH_ID gid = u32(-1);
		//			Msg("[%d]",gid);
		//			Fvector p = ai().game_graph().vertex(gid)->level_point();
		//			Msg("lch_name=%s pos=%f %f %f",*ai().game_graph().header().level(ai().game_graph().vertex(gid)->level_id()).name(), p.x, p.y, p.z);
		//		}


		//	};
		//	if(bDone)
		//	{
		//		Fvector2 position;
		//		position.set			((*lit)->Position().x, (*lit)->Position().z);
		//		m_position_on_map		= map->ConvertRealToLocal(position, false);
		//		UpdateSpotPointer		(map, GetSpotPointer(sp));
		//	}
		//	else
		//	{
		//		xr_vector<u32>::reverse_iterator it = map_point_path.rbegin();
		//		xr_vector<u32>::reverse_iterator it_e = map_point_path.rend();
		//		for(; (it!=it_e)&&(!bDone) ;++it)
		//		{
		//			if(*ai().game_graph().header().level(ai().game_graph().vertex(*it)->level_id()).name()==Level().name())
		//				break;
		//		}
		//		if(it!=it_e)
		//		{
		//			Fvector p = ai().game_graph().vertex(*it)->level_point();
		//			if(Actor()->Position().distance_to_sqr(p)>45.0f*45.0f)
		//			{
		//				Fvector2 position;
		//				position.set			(p.x, p.z);
		//				m_position_on_map		= map->ConvertRealToLocal(position, false);
		//				UpdateSpotPointer		(map, GetSpotPointer(sp));
		//			}
		//		}
		//	}
		//}
	}


}

void CMapLocation::UpdateSpotPointer(CUICustomMap* map, CMapSpotPointer* sp )
{
	if(sp->GetParent()) return ;// already is child
	float		heading;
	Fvector2	pointer_pos;
	if( map->GetPointerTo(m_position_on_map, sp->GetWidth()/2, pointer_pos, heading) )
	{
		sp->SetWndPos(pointer_pos);
		sp->SetHeading(heading);

		map->AttachChild(sp);

		Fvector2 tt = map->ConvertLocalToReal(m_position_on_map, map->BoundRect());
		Fvector ttt;
		ttt.set		(tt.x, 0.0f, tt.y);
	}
}

void CMapLocation::UpdateMiniMap(CUICustomMap* map)
{
	CMapSpot* sp = m_minimap_spot;
	if(!sp) return;
	if(SpotEnabled())
		UpdateSpot(map, sp);

}

void CMapLocation::UpdateLevelMap(CUICustomMap* map)
{
	CComplexMapSpot* csp = m_complex_spot;
	if ( csp && SpotEnabled() )
	{
		UpdateSpot(map, csp);
		return;
	}

	CMapSpot* sp = m_level_spot;
	if ( sp && SpotEnabled() )
	{
		UpdateSpot(map, sp);
	}
}


void CMapLocation::save(IWriter &stream)
{
	stream.w_stringZ(m_hint);
	stream.w_u32	(m_flags.flags);
	stream.w_stringZ(m_owner_task_id);
}

void CMapLocation::load(IReader &stream)
{
	xr_string		str;
	stream.r_stringZ(str);
	SetHint			(str.c_str());
	m_flags.flags	= stream.r_u32	();

	stream.r_stringZ(str);
	m_owner_task_id	= str.c_str();
}

void CMapLocation::SetHint(const shared_str& hint)		
{
	if ( hint == "disable_hint" )
	{
		m_flags.set(eHintEnabled, FALSE);
		m_hint		= "" ;
		return;
	}
	m_hint = hint;
};

LPCSTR CMapLocation::GetHint()
{
	if ( !HintEnabled() ) 
	{
		return NULL;
	}
	return CStringTable().translate(m_hint).c_str();
};

CMapSpotPointer* CMapLocation::GetSpotPointer(CMapSpot* sp)
{
	R_ASSERT( sp );
	if ( !PointerEnabled() )
	{
		return NULL;
	}
	if ( sp == m_level_spot)
	{
		return m_level_spot_pointer;
	}
	else if ( sp == m_minimap_spot)
	{
		return m_minimap_spot_pointer;
	}
	else if ( sp == m_complex_spot)
	{
		return m_complex_spot_pointer;
	}

	return NULL;
}

CMapSpot* CMapLocation::GetSpotBorder(CMapSpot* sp)
{
	R_ASSERT(sp);
	if ( PointerEnabled() )
	{
		if( sp == m_level_spot )
		{
			if ( NULL == m_level_map_spot_border )
			{
				m_level_map_spot_border			= xr_new<CMapSpot>(this);
				m_level_map_spot_border->Load	(g_uiSpotXml,m_spot_border_names[0].c_str());
			}
			return m_level_map_spot_border;
		}
		else if( sp == m_minimap_spot )
		{
			if ( NULL == m_mini_map_spot_border )
			{
				m_mini_map_spot_border			= xr_new<CMapSpot>(this);
				m_mini_map_spot_border->Load	(g_uiSpotXml,m_spot_border_names[2].c_str());
			}
			return m_mini_map_spot_border;
		}
		else if( sp == m_complex_spot )
		{
			if ( NULL == m_complex_spot_border )
			{
				m_complex_spot_border			= xr_new<CMapSpot>(this);
				m_complex_spot_border->Load		(g_uiSpotXml,m_spot_border_names[4].c_str());
			}
			return m_complex_spot_border;
		}
	}
	else
	{// inactive state
		if ( sp == m_level_spot )
		{
			if ( NULL == m_level_map_spot_border_na && m_spot_border_names[1].size() )
			{
				m_level_map_spot_border_na			= xr_new<CMapSpot>(this);
				m_level_map_spot_border_na->Load	(g_uiSpotXml,m_spot_border_names[1].c_str());
			}
			return m_level_map_spot_border_na;
		}
		else if ( sp == m_minimap_spot )
		{
			if ( NULL == m_mini_map_spot_border_na && m_spot_border_names[3].size() )
			{
				m_mini_map_spot_border_na			= xr_new<CMapSpot>(this);
				m_mini_map_spot_border_na->Load		(g_uiSpotXml,m_spot_border_names[3].c_str());
			}
			return m_mini_map_spot_border_na;
		}
		else if ( sp == m_complex_spot )
		{
			if ( NULL == m_complex_spot_border_na && m_spot_border_names[5].size() )
			{
				m_complex_spot_border_na			= xr_new<CMapSpot>(this);
				m_complex_spot_border_na->Load		(g_uiSpotXml,m_spot_border_names[5].c_str());
			}
			return m_complex_spot_border_na;
		}
	}

	return NULL;
}

