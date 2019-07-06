#include "stdafx.h"
#include "object_factory.h"
#include "string_table.h"

#include "entity_alive.h"
#include "ui/UIInventoryUtilities.h"
#include "UI/UIXmlInit.h"
#include "UI/UItextureMaster.h"

#include "profiler.h"

typedef xr_vector<std::pair<shared_str,int> >	STORY_PAIRS;
extern STORY_PAIRS								story_ids;
extern STORY_PAIRS								spawn_story_ids;

extern void show_smart_cast_stats					();
extern void clear_smart_cast_stats					();
extern void release_smart_cast_stats				();
extern void dump_list_wnd							();
extern void dump_list_lines							();
extern void dump_list_sublines						();
extern void clean_wnd_rects							();
extern void dump_list_xmls							();
extern void CreateUIGeom							();
extern void DestroyUIGeom							();
extern void InitHudSoundSettings					();

#include "../xrEngine/IGame_Persistent.h"
void init_game_globals()
{
	if(!g_dedicated_server)
	{
		CreateUIGeom								();
		InitHudSoundSettings						();
		InventoryUtilities::CreateShaders			();

		FS_FileSet fset;
		FS.file_list(fset, "$game_config$", FS_ListFiles,"ui\\textures_descr\\*.xml");
		FS_FileSetIt fit	= fset.begin();
		FS_FileSetIt fit_e	= fset.end();

		for( ;fit!=fit_e; ++fit)
		{
    		string_path	fn1, fn2,fn3;
			_splitpath	((*fit).name.c_str(),fn1,fn2,fn3,0);
			xr_strcat(fn3,".xml");

			CUITextureMaster::ParseShTexInfo(fn3);
		}
	}
}

extern CUIXml*	g_uiSpotXml;
extern CUIXml*	pWpnScopeXml;
//
//void _destroy_item_data_vector_cont(T_VECTOR* vec)
//{
//	T_VECTOR::iterator it		= vec->begin();
//	T_VECTOR::iterator it_e		= vec->end();
//
//	xr_vector<CUIXml*>			_tmp;	
//	for(;it!=it_e;++it)
//	{
//		xr_vector<CUIXml*>::iterator it_f = std::find(_tmp.begin(), _tmp.end(), (*it)._xml);
//		if(it_f==_tmp.end())
//			_tmp.push_back	((*it)._xml);
//	}
//	delete_data	(_tmp);
//}

void clean_game_globals()
{
	// destroy object factory
	xr_delete										(g_object_factory);

	if(!g_dedicated_server)
	{
		InventoryUtilities::DestroyShaders				();
	}
	

	//static shader for blood
	CEntityAlive::UnloadBloodyWallmarks				();
	CEntityAlive::UnloadFireParticles				();
	//очищение памяти таблицы строк
	CStringTable::Destroy							();
	// Очищение таблицы цветов
	CUIXmlInit::DeleteColorDefs						();
	
#ifdef DEBUG
	xr_delete										(g_profiler);
	release_smart_cast_stats						();
#endif

	dump_list_wnd									();
	dump_list_lines									();
	dump_list_sublines								();
	clean_wnd_rects									();
	xr_delete										(g_uiSpotXml);
	dump_list_xmls									();
	DestroyUIGeom									();
	xr_delete										(pWpnScopeXml);
	CUITextureMaster::FreeTexInfo					();
}