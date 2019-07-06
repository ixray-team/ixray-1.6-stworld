#include "stdafx.h"

#include "level.h"
#include "../xrEngine/x_ray.h"
#include "../xrEngine/igame_persistent.h"

#include "game_cl_base.h"
#include "NET_Queue.h"
#include "file_transfer.h"
#include "hudmanager.h"
#include "RegistryFuncs.h"
#include "player_name_modifyer.h"

#include "../xrphysics/iphworld.h"
#include "xrServer.h"


#include "phcommander.h"
#include "physics_game.h"

extern	pureFrame*		g_pNetProcessor;

void GetPlayerName_FromRegistry(char* name, u32 const name_size)
{
	string256	new_name;
	if (!ReadRegistry_StrValue(REGISTRY_VALUE_USERNAME, name))
	{
		name[0] = 0;
		Msg( "! Player name registry key (%s) not found !", REGISTRY_VALUE_USERNAME );
		return;
	}
	u32 const max_name_length	=	GP_UNIQUENICK_LEN - 1;
	if ( xr_strlen(name) > max_name_length )
	{
		name[max_name_length] = 0;
	}
	if ( xr_strlen(name) == 0 )
	{
		Msg( "! Player name in registry is empty! (%s)", REGISTRY_VALUE_USERNAME );
	}
	modify_player_name	(name, new_name);
	strncpy_s(name, name_size, new_name, max_name_length);
}

void WritePlayerName_ToRegistry(LPSTR name)
{
	u32 const max_name_length	=	GP_UNIQUENICK_LEN - 1;
	if ( xr_strlen(name) > max_name_length )
	{
		name[max_name_length] = 0;
	}
	WriteRegistry_StrValue(REGISTRY_VALUE_USERNAME, name);
}


bool CLevel::net_start_client1( )
{
	pApp->LoadBegin				();
	g_pGamePersistent->LoadTitle();
	return true;
}

bool CLevel::net_start_client2( )
{
	m_variables.connected_to_server = Connect2Server(*m_caClientOptions);

	return true;
}
void rescan_mp_archives()
{
	FS_Path* mp_archs_path = FS.get_path("$game_arch_mp$");
	FS.rescan_path(mp_archs_path->m_Path,
		mp_archs_path->m_Flags.is(FS_Path::flRecurse)
	);
}

bool CLevel::net_start_client3( )
{
	if(m_variables.connected_to_server)
	{
		LPCSTR					level_name = NULL;
		LPCSTR					level_ver = NULL;
		LPCSTR					download_url = NULL;

		level_name		= get_net_DescriptionData().map_name;
		level_ver		= get_net_DescriptionData().map_version;
		download_url	= get_net_DescriptionData().download_url;
		rescan_mp_archives();

		// Determine internal level-ID
		int				level_id = pApp->Level_ID(level_name, level_ver, true);
		if (level_id==-1)	
		{
			Disconnect						( );

			m_variables.connected_to_server				= FALSE;
			Msg("! Level (name:%s), (version:%s), not found, try to download from:%s",
				level_name, level_ver, download_url);
			map_data.m_name					= level_name;
			map_data.m_map_version			= level_ver;
			map_data.m_map_download_url		= download_url;
			map_data.m_map_loaded			= false;
			return false;
		}

		map_data.m_name				= level_name;
		map_data.m_map_version		= level_ver;
		map_data.m_map_download_url	= download_url;
		map_data.m_map_loaded		= true;
		
		m_variables.deny_m_spawn	= FALSE;
		// Load level
		BOOL load_result			= Load(level_id);
		R_ASSERT2					( load_result, "Loading failed.");
		map_data.m_level_geom_crc32 = 0;
		CalculateLevelCrc32			();
	}
	return true;
}

bool CLevel::net_start_client4( )
{
	if(m_variables.connected_to_server)
	{
		g_pGamePersistent->LoadTitle		();

		// Send physics to single or multithreaded mode
		create_physics_world	(!!psDeviceFlags.test(mtPhysics), &ObjectSpace, &Objects, &Device);

		R_ASSERT				(physics_world());

		m_ph_commander_physics_worldstep	= xr_new<CPHCommander>();
		physics_world()->set_update_callback( m_ph_commander_physics_worldstep );

		physics_world()->set_default_contact_shotmark( ContactShotMark );
		physics_world()->set_default_character_contact_shotmark( CharacterContactShotMark );

		VERIFY						( physics_world() );
		physics_world()->set_step_time_callback( (PhysicsStepTimeCallback*) &PhisStepsCallback );


		// Send network to single or multithreaded mode
		// *note: release version always has "mt_*" enabled
		Device.seqFrameMT.Remove			(g_pNetProcessor);
		Device.seqFrame.Remove				(g_pNetProcessor);
		
		if (psDeviceFlags.test(mtNetwork))	
			Device.seqFrameMT.Add	(g_pNetProcessor,REG_PRIORITY_HIGH	+ 2);
		else								
			Device.seqFrame.Add		(g_pNetProcessor,REG_PRIORITY_LOW	- 2);

		// Waiting for connection/configuration completition
		CTimer	timer_sync	;	
		timer_sync.Start	();

		while	(!net_isCompleted_Connect())	
			Sleep	(5);

		Msg		("* connection sync: %d ms", timer_sync.GetElapsed_ms());
		while	(!net_isCompleted_Sync())	
		{ 
			ClientReceive(); 
			Sleep(5); 
		}
	}
	return true;
}

void CLevel::ClientSendProfileData( )
{
	NET_Packet				NP;
	NP.w_begin				(M_CREATE_PLAYER_STATE);
	game_PlayerState ps		(NULL);


	if(!g_dedicated_server)
	{
		string64	player_name;
		GetPlayerName_FromRegistry( player_name, sizeof(player_name) );

		if ( xr_strlen(player_name) == 0 )
			xr_strcpy( player_name, xr_strlen(Core.UserName) ? Core.UserName : Core.CompName );
		VERIFY( xr_strlen(player_name) );

		ps.setName				( player_name);
	}

	ps.net_Export				(NP, TRUE);
	SecureSend					(NP,net_flags(TRUE, TRUE, TRUE, TRUE));
}


bool CLevel::net_start_client5( )
{
	if(m_variables.connected_to_server)
	{
		if	(!g_dedicated_server)
		{
			g_pGamePersistent->LoadTitle		();
			Device.m_pRender->DeferredLoad		(FALSE);
			Device.m_pRender->ResourcesDeferredUpload();
			LL_CheckTextures					();
		}
		m_variables.sended_request_connection_data	= FALSE;
		m_variables.deny_m_spawn					= TRUE;
	}
	return true;
}

bool CLevel::net_start_client6( )
{
	if (m_variables.connected_to_server) 
	{
		// Sync
		if (!synchronize_map_data())
			return false;

		if (!m_variables.m_game_configured)
		{
			pApp->LoadEnd					( ); 
			return true;
		}
		if (!g_dedicated_server)
		{
			g_hud->Load						( );
			g_hud->OnConnected				( );
		}

		if(game)
		{
			game->OnConnected				( );
			m_file_transfer = xr_new<file_transfer::client_site>();
		}

		g_pGamePersistent->LoadTitle		( );
		Device.PreCache						( 60, true, true );
		m_variables.net_start_result_total				= TRUE;

	}else
	{
		m_variables.net_start_result_total				= FALSE;
	}

	pApp->LoadEnd							( ); 
	return true;
}
