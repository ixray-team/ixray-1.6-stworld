#include "stdafx.h"
#include "xrserver.h"
#include "game_sv_deathmatch.h"
#include "game_sv_teamdeathmatch.h"
#include "game_sv_artefacthunt.h"
#include "xrMessages.h"
#include "game_cl_artefacthunt.h"
#include "../xrEngine/x_ray.h"
#include "file_transfer.h"
#include "screenshot_server.h"
#include "../xrNetServer/NET_AuthCheck.h"
#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

LPCSTR xrServer::get_map_download_url(LPCSTR level_name, LPCSTR level_version)
{
	R_ASSERT(level_name && level_version);
	LPCSTR ret_url = "";
	CInifile* level_ini = pApp->GetArchiveHeader(level_name, level_version);
	if (!level_ini)
	{
		Msg("! Warning: level [%s][%s] has not header ltx", level_name, level_version);

		return ret_url;
	}

	ret_url = level_ini->r_string_wb("header", "link").c_str();
	if (!ret_url)
		ret_url = "";
	
	return ret_url;
}

xrServer::EConnect xrServer::Connect(shared_str& server_options, GameDescriptionData& game_descr)
{
#ifdef DEBUG
	Msg						("* sv_Connect: %s",	*server_options);
#endif

	//// Parse options and create game
	if (0==strchr(*server_options,'/'))
		return				ErrConnect;

	string1024				options;
	R_ASSERT2(xr_strlen(server_options) <= sizeof(options), "session_name too BIIIGGG!!!");
	xr_strcpy					(options,strchr(*server_options,'/')+1);
	
	// Parse game type
	string1024				type;
	R_ASSERT2(xr_strlen(options) <= sizeof(type), "session_name too BIIIGGG!!!");
	xr_strcpy					(type,options);
	if (strchr(type,'/'))	*strchr(type,'/') = 0;
	game					= NULL;

	CLASS_ID clsid			= game_GameState::getCLASS_ID(type,true);
	game					= smart_cast<game_sv_GameState*> (NEW_INSTANCE(clsid));
	
	// Options
	if (0==game)			return ErrConnect;
//	game->type				= type_id;

	m_file_transfers	= xr_new<file_transfer::server_site>();
	initialize_screenshot_proxies();

	//xr_auth_strings_t	tmp_ignore;
	//xr_auth_strings_t	tmp_check;
	//fill_auth_check_params	(tmp_ignore, tmp_check);
	//FS.auth_generate		(tmp_ignore, tmp_check);

#ifdef DEBUG
	Msg("* Created server_game %s",g_pGamePersistent->GameTypeStr());
#endif
	
	ZeroMemory(&game_descr, sizeof(game_descr));
	xr_strcpy(game_descr.map_name, game->level_name(server_options.c_str()).c_str());
	xr_strcpy(game_descr.map_version, game_sv_GameState::parse_level_version(server_options.c_str()).c_str());
	xr_strcpy(game_descr.download_url, get_map_download_url(game_descr.map_name, game_descr.map_version));

	game->Create			(server_options);

	return IPureServer::Connect(*server_options, game_descr);
}


IClient* xrServer::new_client( SClientConnectData* cl_data )
{
	IClient* CL		= client_Find_Get( cl_data->clientID );
	VERIFY( CL );
	
	// copy entity
	CL->ID			= cl_data->clientID;
	CL->process_id	= cl_data->process_id;
	CL->name		= cl_data->name;	//only for offline mode

	NET_Packet		P;
	P.B.count		= 0;
	P.r_pos			= 0;

	game->AddDelayedEvent( P, GAME_EVENT_CREATE_CLIENT, 0, CL->ID );
	
	return CL;
}

void xrServer::AttachNewClient(IClient* CL)
{
	MSYS_CONFIG	msgConfig;
	msgConfig.sign1 = 0x12071980;
	msgConfig.sign2 = 0x26111975;

	SendTo_LL				(CL->ID,&msgConfig,sizeof(msgConfig), net_flags(TRUE, TRUE, TRUE, TRUE));
	Server_Client_Check		(CL); 


	NeedToCheckClient_BuildVersion	(CL);
	Check_BuildVersion_Success		(CL);

	//// gen message
	//if (!NeedToCheckClient_GameSpy_CDKey(CL))
	//	Check_GameSpy_CDKey_Success(CL);

	//xrClientData * CL_D=(xrClientData*)(CL); 
	//ip_address				ClAddress;
	//GetClientAddress		(CL->ID, ClAddress);
	CL->m_guid[0]=0;
}

//void xrServer::RequestClientDigest(IClient* CL)
//{
//	if (CL == GetServerClient())
//	{
//		Check_BuildVersion_Success(CL);	
//		return;
//	}
//	xrClientData* tmp_client	= smart_cast<xrClientData*>(CL);
//	VERIFY						(tmp_client);
//	PerformSecretKeysSync		(tmp_client);
//
//	NET_Packet P;
//	P.w_begin					(M_SV_DIGEST);
//	SendTo						(CL->ID, P);
//}
//
//void xrServer::ProcessClientDigest(xrClientData* xrCL, NET_Packet* P)
//{
//	R_ASSERT(xrCL);
//	IClient* tmp_client = static_cast<IClient*>(xrCL);
//	game_sv_mp* server_game = smart_cast<game_sv_mp*>(game);
//	P->r_stringZ(xrCL->m_cdkey_digest);
//	GetPooledState				(xrCL);
//	PerformSecretKeysSync		(xrCL);
//	Check_BuildVersion_Success	(tmp_client);	
//}