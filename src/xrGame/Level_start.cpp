#include "stdafx.h"
#include "level.h"
#include "Level_Bullet_Manager.h"
#include "xrserver.h"
#include "game_cl_base.h"
#include "xrmessages.h"
#include "../xrEngine/x_ray.h"
#include "../xrEngine/device.h"
#include "../xrEngine/IGame_Persistent.h"
#include "../xrEngine/xr_ioc_cmd.h"
#include "string_table.h"
#include "UIGameCustom.h"

extern XRCORE_API bool g_allow_heap_min;


BOOL CLevel::net_Start( LPCSTR op_server, LPCSTR op_client )
{
	m_variables.net_start_result_total				= TRUE;

	pApp->LoadBegin				();

	m_caClientOptions			= op_client;
	m_caServerOptions			  = op_server;


	//---------------------------------------------------------------------------
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server1));
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server2));
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server3));
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server4));
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server5));
	g_loading_events.push_back	(LOADING_EVENT(this,&CLevel::net_start_server6));
	
	return m_variables.net_start_result_total;
}

shared_str level_version(const shared_str &server_options);
shared_str level_name(const shared_str &server_options);

bool CLevel::net_start_server1( )
{
	// Start client and server if need it
	if (m_caServerOptions.size())
	{
		g_allow_heap_min		= false;
//.		Server					= xr_new<xrGameSpyServer>();
		Server					= xr_new<xrServer>();

		shared_str l_ver			= game_sv_GameState::parse_level_version(m_caServerOptions);
		
		map_data.m_name				= game_sv_GameState::parse_level_name(m_caServerOptions);
		
		if (!g_dedicated_server)
			g_pGamePersistent->LoadTitle(true, map_data.m_name);

		int							id = pApp->Level_ID(map_data.m_name.c_str(), l_ver.c_str(), true);

		if (id<0) 
		{
			Log						("Can't find level: ",map_data.m_name.c_str());
			m_variables.net_start_result_total	= FALSE;
			return true;
		}
	} else
	{
		g_allow_heap_min = false;
	}
	return true;
}

bool CLevel::net_start_server2( )
{
	if (m_variables.net_start_result_total && m_caServerOptions.size())
	{
		GameDescriptionData game_descr;
		if ((m_variables.m_connect_server_err=Server->Connect(m_caServerOptions, game_descr))!=xrServer::ErrNoError)
		{
			m_variables.net_start_result_total = false;
			Msg				("! Failed to start server.");
			return true;
		}
		Server->SLS_Default		();
		map_data.m_name			= Server->level_name(m_caServerOptions);
		if (!g_dedicated_server)
			g_pGamePersistent->LoadTitle(true, map_data.m_name);
	}
	return true;
}

bool CLevel::net_start_server3( )
{
	if(!m_variables.net_start_result_total) return true;
	//add server port if don't have one in options
	if (!strstr(m_caClientOptions.c_str(), "port=") && Server)
	{
		string64	PortStr;
		xr_sprintf(PortStr, "/port=%d", Server->GetPort());

		string4096	tmp;
		xr_strcpy(tmp, m_caClientOptions.c_str());
		xr_strcat(tmp, PortStr);
		
		m_caClientOptions = tmp;
	}
	//add password string to client, if don't have one
	if(m_caServerOptions.size())
	{
		if (strstr(m_caServerOptions.c_str(), "psw=") && !strstr(m_caClientOptions.c_str(), "psw="))
		{
			string64	PasswordStr = "";
			const char* PSW = strstr(m_caServerOptions.c_str(), "psw=") + 4;
			if (strchr(PSW, '/')) 
				strncpy_s(PasswordStr, PSW, strchr(PSW, '/') - PSW);
			else
				xr_strcpy(PasswordStr, PSW);

			string4096	tmp;
			xr_sprintf(tmp, "%s/psw=%s", m_caClientOptions.c_str(), PasswordStr);
			m_caClientOptions = tmp;
		};
	};
	return true;
}

bool CLevel::net_start_server4( )
{
	if(!m_variables.net_start_result_total) return true;

	g_loading_events.pop_front();

	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client6));
	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client5));
	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client4));
	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client3));
	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client2));
	g_loading_events.push_front	(LOADING_EVENT(this,&CLevel::net_start_client1));

	return false;
}

bool CLevel::net_start_server5( )
{
	if(m_variables.net_start_result_total)
	{
		NET_Packet		NP;
		NP.w_begin		(M_CLIENTREADY);
		Game().local_player->net_Export(NP, TRUE);
		Send			(NP,net_flags(TRUE,TRUE));
	};
	return true;
}

bool CLevel::net_start_server6( )
{
	//init bullet manager
	BulletManager().Clear		();
	BulletManager().Load		();

	pApp->LoadEnd				();

	if(!m_variables.net_start_result_total)
	{
		Msg("! Failed to start client. Check the connection or level existance.");
		
		if (m_variables.m_connect_server_err==xrServer::ErrConnect && !g_dedicated_server) 
		{
			DEL_INSTANCE	(g_pGameLevel);
			pConsoleCommands->Execute("main_menu on");
		}else
		if (!map_data.m_map_loaded && map_data.m_name.size() && m_variables.m_bConnectResult)
		{
			LPCSTR level_id_string = NULL;
			LPCSTR dialog_string = NULL;
			LPCSTR download_url = !!map_data.m_map_download_url ? map_data.m_map_download_url.c_str() : "";
			CStringTable	st;
			LPCSTR tmp_map_ver = !!map_data.m_map_version ? map_data.m_map_version.c_str() : "";
			
			STRCONCAT(level_id_string, st.translate("st_level"), ":",
				map_data.m_name.c_str(), "(", tmp_map_ver, "). ");
			STRCONCAT(dialog_string, level_id_string, st.translate("ui_st_map_not_found"));

			DEL_INSTANCE	(g_pGameLevel);
			pConsoleCommands->Execute("main_menu on");

		}else
		if (map_data.IsInvalidClientChecksum())
		{
			LPCSTR level_id_string = NULL;
			LPCSTR dialog_string = NULL;
			LPCSTR download_url = !!map_data.m_map_download_url ? map_data.m_map_download_url.c_str() : "";
			CStringTable	st;
			LPCSTR tmp_map_ver = !!map_data.m_map_version ? map_data.m_map_version.c_str() : "";

			STRCONCAT(level_id_string, st.translate("st_level"), ":",
				map_data.m_name.c_str(), "(", tmp_map_ver, "). ");
			STRCONCAT(dialog_string, level_id_string, st.translate("ui_st_map_data_corrupted"));

			g_pGameLevel->net_Stop();
			DEL_INSTANCE	(g_pGameLevel);
			pConsoleCommands->Execute("main_menu on");
		}else 
		{
			DEL_INSTANCE	(g_pGameLevel);
			pConsoleCommands->Execute("main_menu on");
		}

		return true;
	}

	if(!g_dedicated_server)
	{
		if (CurrentGameUI())
			CurrentGameUI()->OnConnected();
	}

	return true;
}

void CLevel::InitializeClientGame( NET_Packet& P )
{
	string256 game_type_name;
	P.r_stringZ(game_type_name);
	
	xr_delete(game);
#ifdef DEBUG
	Msg("- Game configuring : Started ");
#endif // #ifdef DEBUG
	CLASS_ID clsid			= game_GameState::getCLASS_ID(game_type_name,false);
	game					= smart_cast<game_cl_GameState*> ( NEW_INSTANCE ( clsid ) );
	
	g_pGamePersistent->m_e_game_type = IGame_Persistent::ParseStringToGameType( game_type_name );
	game->Init				();
	m_variables.m_game_config_started	= TRUE;

	init_compression		();
	
	R_ASSERT				(Load_GameSpecific_After ());
}

