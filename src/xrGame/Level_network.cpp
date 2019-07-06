#include "stdafx.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "game_cl_base.h"
#include "PHCommander.h"
#include "net_queue.h"
//#include "lobby_menu.h"
#include "UIGameCustom.h"
#include "string_table.h"
#include "file_transfer.h"
#include "UI/UIGameTutorial.h"
#include "../xrNetServer/NET_AuthCheck.h"

#include "../xrphysics/physicscommon.h"
ENGINE_API bool g_dedicated_server;

const int max_objects_size			= 2*1024;
const int max_objects_size_in_save	= 8*1024;

extern bool	g_b_ClearGameCaptions;

void CLevel::remove_objects( )
{
	Msg							("CLevel::remove_objects - Start");
	BOOL b_stored = psDeviceFlags.test(rsDisableObjectsAsCrows);
	
	int loop = 5;
	while(loop)
	{
		if (OnServer()) 
		{
			R_ASSERT				(Server);
			Server->SLS_Clear		();
		}

		if (OnClient())
			ClearAllObjects			();

		for (int i=0; i<20; ++i) 
		{
			snd_Events.clear		();
			psNET_Flags.set			(NETFLAG_MINIMIZEUPDATES,FALSE);
			// ugly hack for checks that update is twice on frame
			// we need it since we do updates for checking network messages
			++(Device.dwFrame);
			psDeviceFlags.set		(rsDisableObjectsAsCrows,TRUE);
			ClientReceive			();
			ProcessGameEvents		();
			Objects.Update			(false);
			Objects.dump_all_objects();
		}

		if(Objects.o_count()==0)
			break;
		else
		{
			--loop;
			Msg("Objects removal next loop. Active objects count=%d", Objects.o_count());
		}

	}

	BulletManager().Clear		();
	ph_commander().clear		();

	psDeviceFlags.set			(rsDisableObjectsAsCrows, b_stored);
	g_b_ClearGameCaptions		= true;
	
	VERIFY										(Render);
	Render->ClearPool							(FALSE);
	
	Render->clear_static_wallmarks				();

	g_pGamePersistent->destroy_particles		(false);
}

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

void CLevel::net_Stop( )
{
	Msg							("- Disconnect");

	if(CurrentGameUI())
	{
		CurrentGameUI()->HideShownDialogs();
	}

	if(g_tutorial && !g_tutorial->Persistent())
		g_tutorial->Stop();

	if(g_tutorial2 && !g_tutorial->Persistent())
		g_tutorial2->Stop();

	bReady						= false;
	m_variables.m_game_config_started		= FALSE;

	if (m_file_transfer)
		xr_delete(m_file_transfer);

	remove_objects				();
	
	//WARNING ! remove_objects() uses this flag, so position of this line must e here ..
	m_variables.m_game_configured			= FALSE;
	
	IGame_Level::net_Stop		();
	IPureClient::Disconnect		();

	if (Server) 
	{
		Server->Disconnect		();
		xr_delete				(Server);
	}
}


void CLevel::ClientSend()
{
	if (OnClient())
	{
		if ( !net_HasBandwidth() ) return;
	};

	NET_Packet				P;
	u32						start	= 0;
	//----------- for E3 -----------------------------
//	if () 
	{
//		if (!(Game().local_player) || Game().local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) return;
		if (CurrentControlEntity()) 
		{
			CObject* pObj = CurrentControlEntity();
			if (!pObj->getDestroy() && pObj->net_Relevant())
			{				
				P.w_begin		(M_CL_UPDATE);
				

				P.w_u16			(u16(pObj->ID()));
				P.w_u32			(0);	//reserved place for client's ping

				pObj->net_Export			(P);

				if (P.B.count>9)				
				{
					if (!OnServer())
						Send	(P, net_flags(FALSE));
				}				
			}			
		}		
	};
	if (m_file_transfer)
	{
		m_file_transfer->update_transfer();
		m_file_transfer->stop_obsolete_receivers();
	}
	if (OnClient()) 
	{
		Flush_Send_Buffer();
		return;
	}
	//-------------------------------------------------
	while (1)
	{
		P.w_begin						(M_UPDATE);
		start	= Objects.net_Export	(&P, start, max_objects_size);

		if (P.B.count>2)
		{
			Device.Statistic->TEST3.Begin();
				Send	(P, net_flags(FALSE));
			Device.Statistic->TEST3.End();
		}else
			break;
	}
}


void CLevel::Send(NET_Packet& P, u32 dwFlags)
{
//	if (IsDemoPlayStarted() || IsDemoPlayFinished()) return;

	if(Server && m_variables.m_game_configured && OnServer() )
	{
		Server->OnMessageSync	(P,Game().local_svdpnid	);
	}else											
		IPureClient::Send	(P,dwFlags);
}

void CLevel::net_Update	()
{
	if(m_variables.m_game_configured)
	{
		// If we have enought bandwidth - replicate client data on to server
		Device.Statistic->netClient2.Begin	();
		ClientSend					();
		Device.Statistic->netClient2.End		();
	}
	// If server - perform server-update
	if (Server && OnServer())	
	{
		Device.Statistic->netServer.Begin();
		Server->Update					();
		Device.Statistic->netServer.End	();
	}
}

struct _NetworkProcessor	: public pureFrame
{
	virtual void	_BCL OnFrame	( )
	{
		if (g_pGameLevel && !Device.Paused() )	g_pGameLevel->net_Update();
	}
}NET_processor;

pureFrame*	g_pNetProcessor	= &NET_processor;

const int ConnectionTimeOutMs = 60000;//60000;

BOOL CLevel::Connect2Server( LPCSTR options )
{
	NET_Packet					P;
	m_variables.m_bConnectResultReceived	= false	;
	m_variables.m_bConnectResult			= true	;

	if (!Connect(options))		
		return	FALSE;


	u32 EndTime = GetTickCount() + ConnectionTimeOutMs;

	while(!m_variables.m_bConnectResultReceived)		
	{ 
		ClientReceive	();
		Sleep			(5); 
		if(Server)
			Server->Update()	;
		//-----------------------------------------
		u32 CurTime = GetTickCount();
		if (CurTime > EndTime)
		{
			NET_Packet	P;
			P.B.count = 0;
			P.r_pos = 0;

			P.w_u8(0);
			P.w_u8(0);
			P.w_stringZ("Data verification failed. Cheater?");

			OnConnectResult(&P);			
		}
		if (net_isFails_Connect())
		{
			OnConnectRejected	();	
			Disconnect		()	;
			return	FALSE;
		}
		//-----------------------------------------
	}
	Msg("%c client : connection %s - <%s>", m_variables.m_bConnectResult ?'*':'!', m_variables.m_bConnectResult ? "accepted" : "rejected", m_variables.m_sConnectResult.c_str());
	
	if(!m_variables.m_bConnectResult) 
	{
		if(Server)
		{
			Server->Disconnect		();
			xr_delete				(Server);
		}
		OnConnectRejected			();	
		Disconnect					();
		return FALSE		;
	};

	
	net_Syncronize	();

	while (!net_IsSyncronised()) 
	{
		Sleep(1);
		if (net_Disconnected)
		{
			OnConnectRejected	();	
			Disconnect			();
			return FALSE;
		}
	};
	return TRUE;
};

void CLevel::OnConnectResult(NET_Packet* P)
{
	// multiple results can be sent during connection they should be "AND-ed"
	m_variables.m_bConnectResultReceived	= true;
	u8	result					= P->r_u8();
	u8  res1					= P->r_u8();
	string512 ResultStr;	
	P->r_stringZ_s				(ResultStr);
	//ClientID tmp_client_id;
	//P->r_clientID				(tmp_client_id);
	//SetClientID					(tmp_client_id);
	if (!result)				
	{
		m_variables.m_bConnectResult	= false			;	
		switch (res1)
		{
		case ecr_data_verification_failed:		//Standart error
			{
				R_ASSERT(0);
				//if (strstr(ResultStr, "Data verification failed. Cheater?"))
				//	MainMenu()->SetErrorDialog(CMainMenu::ErrDifferentVersion);
			}break;
		case ecr_cdkey_validation_failed:		//GameSpy CDKey
			{
				R_ASSERT(0);
				//if (!xr_strcmp(ResultStr, "Invalid CD Key"))
				//	MainMenu()->SetErrorDialog(CMainMenu::ErrCDKeyInvalid);//, ResultStr);
				//if (!xr_strcmp(ResultStr, "CD Key in use"))
				//	MainMenu()->SetErrorDialog(CMainMenu::ErrCDKeyInUse);//, ResultStr);
				//if (!xr_strcmp(ResultStr, "Your CD Key is disabled. Contact customer service."))
				//	MainMenu()->SetErrorDialog(CMainMenu::ErrCDKeyDisabled);//, ResultStr);
			}break;		
		case ecr_password_verification_failed:		//login+password
			{
				R_ASSERT(0);
//				MainMenu()->SetErrorDialog(CMainMenu::ErrInvalidPassword);
			}break;
		case ecr_have_been_banned:
			{
				R_ASSERT(0);
				//if (!xr_strlen(ResultStr))
				//{
				//	MainMenu()->OnSessionTerminate(
				//		CStringTable().translate("st_you_have_been_banned").c_str()
				//	);
				//} else
				//{
				//	MainMenu()->OnSessionTerminate(
				//		CStringTable().translate(ResultStr).c_str()
				//	);
				//}
			}break;
		case ecr_profile_error:
			{
				R_ASSERT(0);
				//if (!xr_strlen(ResultStr))
				//{
				//	MainMenu()->OnSessionTerminate(
				//		CStringTable().translate("st_profile_error").c_str()
				//	);
				//} else
				//{
				//	MainMenu()->OnSessionTerminate(
				//		CStringTable().translate(ResultStr).c_str()
				//	);
				//}
			}
		}
	};	
	m_variables.m_sConnectResult			= ResultStr;
};

void CLevel::ClearAllObjects( )
{
	u32 CLObjNum = Level().Objects.o_count();

	bool ParentFound = true;
	
	while (ParentFound)
	{	
		ParentFound = false;
		for (u32 i=0; i<CLObjNum; i++)
		{
			CObject* pObj = Level().Objects.o_get_by_iterator(i);
			if (!pObj->H_Parent()) continue;
			//-----------------------------------------------------------
			NET_Packet			GEN;
			GEN.w_begin			(M_EVENT);
			//---------------------------------------------		
			GEN.w_u32			(Level().timeServer());
			GEN.w_u16			(GE_OWNERSHIP_REJECT);
			GEN.w_u16			(pObj->H_Parent()->ID());
			GEN.w_u16			(u16(pObj->ID()));
			game_events->insert	(GEN);
			if (g_bDebugEvents)	ProcessGameEvents();
			//-------------------------------------------------------------
			ParentFound = true;
			//-------------------------------------------------------------
#ifdef DEBUG
			Msg ("Rejection of %s[%d] from %s[%d]", *(pObj->cNameSect()), pObj->ID(), *(pObj->H_Parent()->cNameSect()), pObj->H_Parent()->ID());
#endif
		};
		ProcessGameEvents();
	};

	CLObjNum = Level().Objects.o_count();

	for (u32 i=0; i<CLObjNum; i++)
	{
		CObject* pObj = Level().Objects.o_get_by_iterator(i);
		if (pObj->H_Parent() != NULL)
		{
			Msg("! ERROR: object's parent is not NULL");
		}
		
		//-----------------------------------------------------------
		NET_Packet			GEN;
		GEN.w_begin			(M_EVENT);
		//---------------------------------------------		
		GEN.w_u32			(Level().timeServer());
		GEN.w_u16			(GE_DESTROY);
		GEN.w_u16			(u16(pObj->ID()));
		game_events->insert	(GEN);
		if (g_bDebugEvents)	ProcessGameEvents();
		//-------------------------------------------------------------
		ParentFound = true;
		//-------------------------------------------------------------
#ifdef DEBUG
		Msg ("Destruction of %s[%d]", *(pObj->cNameSect()), pObj->ID());
#endif
	};
	ProcessGameEvents();
};

void CLevel::OnInvalidHost( )
{
	IPureClient::OnInvalidHost();
	R_ASSERT(0);
	//if (MainMenu()->GetErrorDialogType() == CMainMenu::ErrNoError)
	//	MainMenu()->SetErrorDialog(CMainMenu::ErrInvalidHost);
};

void CLevel::OnInvalidPassword( )
{
	IPureClient::OnInvalidPassword();
	R_ASSERT(0);
//	MainMenu()->SetErrorDialog(CMainMenu::ErrInvalidPassword);
};

void CLevel::OnSessionFull( )
{
	IPureClient::OnSessionFull();
	R_ASSERT(0);
	//if (MainMenu()->GetErrorDialogType() == CMainMenu::ErrNoError)
	//	MainMenu()->SetErrorDialog(CMainMenu::ErrSessionFull);
}

void CLevel::OnConnectRejected( )
{
	IPureClient::OnConnectRejected();

//	if (MainMenu()->GetErrorDialogType() != CMainMenu::ErrNoError)
//		MainMenu()->SetErrorDialog(CMainMenu::ErrServerReject);
};

