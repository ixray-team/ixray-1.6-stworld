#include "stdafx.h"
#pragma hdrstop

#include "IGame_Persistent.h"
#include "std_classes.h"

#ifndef _EDITOR
#	include "environment.h"
#	include "x_ray.h"
#	include "IGame_Level.h"
#	include "XR_IOConsole.h"
#	include "Render.h"
#	include "ps_instance.h"
#	include "CustomHUD.h"
#	include "xr_ioc_cmd.h"
#endif

#ifdef _EDITOR
	bool g_dedicated_server	= false;
#endif

#ifdef INGAME_EDITOR
#	include "editor_environment_manager.hpp"
#endif // INGAME_EDITOR

ENGINE_API	IGame_Persistent*		g_pGamePersistent	= NULL;

IGame_Persistent::IGame_Persistent	()
:m_e_game_type(eGameIDNoGame)
#ifndef _EDITOR
,pEnvironment(NULL)
#endif //#ifndef _EDITOR
{
	RDEVICE.seqAppStart.Add			(this);
	RDEVICE.seqAppEnd.Add			(this);
	RDEVICE.seqFrame.Add			(this,REG_PRIORITY_HIGH+1);
	RDEVICE.seqAppActivate.Add		(this);
	RDEVICE.seqAppDeactivate.Add	(this);

	m_pMainMenu						= NULL;

#ifndef DEDICATED_SERVER

	#ifndef INGAME_EDITOR
		#ifndef _EDITOR
		pEnvironment					= xr_new<CEnvironment>();
		#endif
	#else // #ifdef INGAME_EDITOR
		if (RDEVICE.editor())
			pEnvironment				= xr_new<editor::environment::manager>();
		else
			pEnvironment				= xr_new<CEnvironment>();
	#endif // #ifdef INGAME_EDITOR
#endif // #ifndef DEDICATED_SERVER
}

IGame_Persistent::~IGame_Persistent	()
{
	RDEVICE.seqFrame.Remove			(this);
	RDEVICE.seqAppStart.Remove		(this);
	RDEVICE.seqAppEnd.Remove			(this);
	RDEVICE.seqAppActivate.Remove	(this);
	RDEVICE.seqAppDeactivate.Remove	(this);
#ifndef _EDITOR
	xr_delete						(pEnvironment);
#endif
}

void IGame_Persistent::OnAppStart	()
{
#ifndef DEDICATED_SERVER
	#ifndef _EDITOR
		Environment().load				();
	#endif    
#endif    
}

void IGame_Persistent::OnAppEnd		()
{
#ifndef DEDICATED_SERVER
	#ifndef _EDITOR
		Environment().unload			 ();
	#endif    
#endif    

	OnGameEnd						();

#ifndef _EDITOR
	DEL_INSTANCE					(g_hud);
#endif    
}

void IGame_Persistent::StartLobby( LPCSTR lobby_menu_name )
{
	if(g_pGameLevel)
	{
		g_pGameLevel->net_Stop	( );
		DEL_INSTANCE			( g_pGameLevel );
		Disconnect				( );
	}
	g_pGameLevel			= (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
	pApp->LoadBegin			( ); 
	m_e_game_type			= eGameIDLobbyMenu;
	Start					( lobby_menu_name, eGameIDLobbyMenu );
	g_pGameLevel->LoadLobbyMenu	(lobby_menu_name);
	pApp->LoadEnd			( ); 
}

void IGame_Persistent::StartNetGame( LPCSTR op_server, LPCSTR op_client )
{
	string256		level_name = "";
	string256		game_mode_str;
	EGameIDs		game_type = eGameIDNoGame;

	// parse cmd_line
	int		n = _GetItemCount(op_server,'/');
	if(n>=2)
	{
		_GetItem			(op_server,0,level_name,'/');
		strlwr				(level_name);

		_GetItem			(op_server,1,game_mode_str,'/');
		strlwr				(game_mode_str);
		game_type			= IGame_Persistent::ParseStringToGameType(game_mode_str);
	}else
	{
		if(op_server)
		{
			string1024 error_msg;
			strconcat(sizeof(error_msg), error_msg, "incorrect server options passed: ", op_server);
			CHECK_OR_EXIT(FALSE,error_msg);
		}
	}

#ifndef DEDICATED_SERVER
	pConsoleCommands->Execute	( "main_menu off" );
	pConsole->Hide				( );
#endif //#ifndef DEDICATED_SERVER

	if(game_type!=GameType()) 
		OnGameEnd	( );

	g_pGameLevel			= (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
	pApp->LoadBegin			( ); 
	Start					( level_name, game_type );
	g_pGameLevel->net_Start	( op_server, op_client[0]?op_client : "localhost" );
	pApp->LoadEnd			( ); 
}

void IGame_Persistent::Start( LPCSTR level_name, EGameIDs game_type )
{
	// change game type
	if(game_type!=m_e_game_type) 
	{
		m_e_game_type		= game_type;
		OnGameStart			( );
#ifndef _EDITOR
		if(g_hud)
			DEL_INSTANCE	( g_hud );
#endif            
	}

	VERIFY					( ps_destroy.empty() );
}

void IGame_Persistent::Disconnect( )
{
#ifndef _EDITOR
	destroy_particles		(true);

	if(g_hud)
			DEL_INSTANCE	(g_hud);
#endif
	m_e_game_type			= eGameIDNoGame;
}

void IGame_Persistent::OnGameStart()
{
#ifndef _EDITOR
	LoadTitle();
	if(!strstr(Core.Params,"-noprefetch"))
		Prefetch();
#endif
}

#ifndef _EDITOR
void IGame_Persistent::Prefetch()
{
	// prefetch game objects & models
	float	p_time						= 1000.f*Device.GetTimerGlobal()->GetElapsed_sec();
	u32	mem_0							= Memory.mem_usage()	;

	Log									("Prefetching models...");
	Render->models_Prefetch				();
	Device.m_pRender->ResourcesDeferredUpload();

	p_time				= 1000.f*Device.GetTimerGlobal()->GetElapsed_sec() - p_time;
	u32		p_mem		= Memory.mem_usage() - mem_0	;

	Msg					("* [prefetch] time:    %d ms",	iFloor(p_time));
	Msg					("* [prefetch] memory:  %dKb",	p_mem/1024);
}
#endif

void IGame_Persistent::OnGameEnd( )
{
#ifndef _EDITOR
	Render->ClearPool		(TRUE);
#endif
}

void IGame_Persistent::OnFrame		()
{
#ifndef DEDICATED_SERVER
#ifndef _EDITOR
	if(!Device.Paused() || Device.dwPrecacheFrame)
		Environment().OnFrame	();


	Device.Statistic->Particles_starting= ps_needtoplay.size	();
	Device.Statistic->Particles_active	= ps_active.size		();
	Device.Statistic->Particles_destroy	= ps_destroy.size		();

	// Play req particle systems
	while (ps_needtoplay.size())
	{
		CPS_Instance*	psi		= ps_needtoplay.back	();
		ps_needtoplay.pop_back	();
		psi->Play				(false);
	}
	// Destroy inactive particle systems
	while (ps_destroy.size())
	{
		CPS_Instance*	psi		= ps_destroy.back();
		VERIFY					(psi);
		if (psi->Locked())
		{
			Log("--locked");
			break;
		}
		ps_destroy.pop_back		();
		psi->PSI_internal_delete();
	}
#endif
#endif //#ifndef DEDICATED_SERVER
}

void IGame_Persistent::destroy_particles(const bool& all_particles)
{
#ifndef _EDITOR
	ps_needtoplay.clear				();

	while (ps_destroy.size())
	{
		CPS_Instance*	psi		= ps_destroy.back	();		
		VERIFY					(psi);
		VERIFY					(!psi->Locked());
		ps_destroy.pop_back		();
		psi->PSI_internal_delete();
	}

	// delete active particles
	if (all_particles) {
		for (;!ps_active.empty();)
			(*ps_active.begin())->PSI_internal_delete	();
	}
	else {
		u32								active_size = ps_active.size();
		CPS_Instance					**I = (CPS_Instance**)_alloca(active_size*sizeof(CPS_Instance*));
		std::copy						(ps_active.begin(),ps_active.end(),I);

		struct destroy_on_game_load {
			static IC bool predicate (CPS_Instance*const& object)
			{
				return					(!object->destroy_on_game_load());
			}
		};

		CPS_Instance					**E = std::remove_if(I,I + active_size,&destroy_on_game_load::predicate);
		for ( ; I != E; ++I)
			(*I)->PSI_internal_delete	();
	}

	VERIFY								(ps_needtoplay.empty() && ps_destroy.empty() && (!all_particles || ps_active.empty()));
#endif
}

void IGame_Persistent::OnAssetsChanged()
{
#ifndef _EDITOR
	Device.m_pRender->OnAssetsChanged();
#endif    
}

LPCSTR GameTypeToString(EGameIDs gt, bool bShort)
{
	switch(gt)
	{
	case eGameIDDeathmatch:
		return (bShort)?"dm":"deathmatch";
		break;
	case eGameIDTeamDeathmatch:
		return (bShort)?"tdm":"teamdeathmatch";
		break;
	case eGameIDArtefactHunt:
		return (bShort)?"ah":"artefacthunt";
		break;
	case eGameIDCaptureTheArtefact:
		return (bShort)?"cta":"capturetheartefact";
		break;
	default :
		return		"---";
	}
}

EGameIDs IGame_Persistent::ParseStringToGameType(LPCSTR str)
{
	if (!xr_strcmp(str, "deathmatch") || !xr_strcmp(str, "dm")) 
		return eGameIDDeathmatch;
	else
	if (!xr_strcmp(str, "teamdeathmatch") || !xr_strcmp(str, "tdm")) 
		return eGameIDTeamDeathmatch;
	else
	if (!xr_strcmp(str, "artefacthunt") || !xr_strcmp(str, "ah")) 
		return eGameIDArtefactHunt;
	else
	if (!xr_strcmp(str, "capturetheartefact") || !xr_strcmp(str, "cta")) 
		return eGameIDCaptureTheArtefact;
	else
	{
		R_ASSERT3(false, "unknown game type:", str);
		return eGameIDNoGame;
	}
}

LPCSTR IGame_Persistent::GameTypeStr( ) const
{
	return GameTypeToString(m_e_game_type, true);
}
