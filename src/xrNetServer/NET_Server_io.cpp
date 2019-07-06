////////////////////////////////////////////////////////////////////////////
//	Created		: 15.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NET_Server.h"
#include "dxerr.h"


void IPureServer::SendTo_LL(ClientID ID, const void* data, u32 size, u32 dwFlags )
{
	OnDataSent			( data, size );

	DPN_BUFFER_DESC		desc;
	desc.dwBufferSize	= size;
	desc.pBufferData	= LPBYTE(data);

	// verify
	VERIFY		(desc.dwBufferSize);
	VERIFY		(desc.pBufferData);

	DPNHANDLE	hAsync	= 0;
	HRESULT		_hr		= NET->SendTo(
		ID.value(),
		&desc,1,
		0/*dwTimeout*/,
		0,&hAsync,
		dwFlags | DPNSEND_COALESCE 
		);

	if (SUCCEEDED(_hr) || (DPNERR_CONNECTIONLOST==_hr))	return;

	R_CHK		(_hr);
}

static HRESULT WINAPI Handler (PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
	IPureServer* C			= (IPureServer*)pvUserContext;
	return C->net_Handler	(dwMessageType,pMessage);
}

// {0218FA8B-515B-4bf2-9A5F-2F079D1759F3}
static const GUID NET_GUID = 
{ 0x218fa8b, 0x515b, 0x4bf2, { 0x9a, 0x5f, 0x2f, 0x7, 0x9d, 0x17, 0x59, 0xf3 } };
// {8D3F9E5E-A3BD-475b-9E49-B0E77139143C}
static const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP =
{ 0x8d3f9e5e, 0xa3bd, 0x475b, { 0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c } };

HRESULT	IPureServer::net_Handler(u32 dwMessageType, PVOID pMessage)
{
    // HRESULT     hr = S_OK;
	
    switch (dwMessageType)
    {
	case DPN_MSGID_ENUM_HOSTS_QUERY :
		{
			PDPNMSG_ENUM_HOSTS_QUERY	msg = PDPNMSG_ENUM_HOSTS_QUERY(pMessage);
			if (0 == msg->dwReceivedDataSize) return S_FALSE;
			if (!stricmp((const char*)msg->pvReceivedData, "ToConnect")) return S_OK;
			if (*((const GUID*) msg->pvReceivedData) != NET_GUID) return S_FALSE;
			if (!OnCL_QueryHost()) return S_FALSE;
			return S_OK;
		}break;
	case DPN_MSGID_CREATE_PLAYER :
        {
			PDPNMSG_CREATE_PLAYER	msg = PDPNMSG_CREATE_PLAYER(pMessage);
			const	u32				max_size = 1024;
			char	bufferData		[max_size];
            DWORD	bufferSize		= max_size;
			ZeroMemory				(bufferData,bufferSize);
			string512				res;

			// retreive info
			DPN_PLAYER_INFO*		Pinfo = (DPN_PLAYER_INFO*) bufferData;
			Pinfo->dwSize			= sizeof(DPN_PLAYER_INFO);
			HRESULT _hr				= NET->GetClientInfo( msg->dpnidPlayer, Pinfo, &bufferSize, 0 );
			if( _hr == DPNERR_INVALIDPLAYER )
			{
				break;	// server player
			}

			CHK_DX					(_hr);
			
			//string64			cname;
			//CHK_DX( WideCharToMultiByte( CP_ACP, 0, Pinfo->pwszName, -1, cname, sizeof(cname) , 0, 0 ) );

			SClientConnectData	cl_data;
			//xr_strcpy( cl_data.name, cname );

			if( Pinfo->pvData && Pinfo->dwDataSize == sizeof(cl_data) )
			{
				cl_data		= *((SClientConnectData*)Pinfo->pvData);
			}
			cl_data.clientID.set( msg->dpnidPlayer );

			new_client( &cl_data );
        }
		break;
	case DPN_MSGID_DESTROY_PLAYER:
		{
			PDPNMSG_DESTROY_PLAYER	msg = PDPNMSG_DESTROY_PLAYER(pMessage);
			IClient* tmp_client = net_players.GetFoundClient(
				ClientIdSearchPredicate(static_cast<ClientID>(msg->dpnidPlayer))
			);
			if (tmp_client)
			{
				tmp_client->flags.bConnected	= FALSE;
				tmp_client->flags.bReconnect	= FALSE;
				OnCL_Disconnected	(tmp_client);
				// real destroy
				client_Destroy		(tmp_client);
			}
		}
		break;
	case DPN_MSGID_RECEIVE:
        {

            PDPNMSG_RECEIVE	pMsg = PDPNMSG_RECEIVE(pMessage);
			void*	m_data		= pMsg->pReceiveData;
			u32		m_size		= pMsg->dwReceiveDataSize;
			DPNID   m_sender	= pMsg->dpnidSender;

			MSYS_PING*	m_ping	= (MSYS_PING*)m_data;
			
			if ((m_size>2*sizeof(u32)) && (m_ping->sign1==0x12071980) && (m_ping->sign2==0x26111975))
			{
				// this is system message
				if (m_size==sizeof(MSYS_PING))
				{
					// ping - save server time and reply
					m_ping->dwTime_Server	= TimerAsync(device_timer);
					ClientID ID; ID.set(m_sender);
					//						IPureServer::SendTo_LL	(ID,m_data,m_size,net_flags(FALSE,FALSE,TRUE));
					IPureServer::SendTo_Buf	(ID,m_data,m_size,net_flags(FALSE,FALSE,TRUE, TRUE));
				}
			} 
			else 
			{
                MultipacketReciever::RecievePacket( pMsg->pReceiveData, pMsg->dwReceiveDataSize, m_sender );
			}
        } break;
        
	case DPN_MSGID_INDICATE_CONNECT :
		{
			//PDPNMSG_INDICATE_CONNECT msg = (PDPNMSG_INDICATE_CONNECT)pMessage;

			//ip_address			HAddr;
			//GetClientAddress	(msg->pAddressPlayer, HAddr);

			//first connected client is SV_Client so if it is NULL then this server client tries to connect ;)
			//if(SV_Client)
			//{
			//	if(is_ip_denied(HAddr))
			//	{
			//		msg->dwReplyDataSize	= sizeof(NET_NOTFOR_SUBNET_STR);
			//		msg->pvReplyData		= NET_NOTFOR_SUBNET_STR;
			//	}
			//	return					S_FALSE;
			//}
		}break;
    }
    return S_OK;
}


IPureServer::EConnect IPureServer::Connect(LPCSTR options, GameDescriptionData & game_descr)
{
	connect_options			= options;

	// Parse options
	string4096				session_name;
	
	string64				password_str = "";
	u32						dwMaxPlayers = 0;
	

	//sertanly we can use game_descr structure for determinig level_name, but for backward compatibility we save next line...
	xr_strcpy					(session_name,options);
	if (strchr(session_name,'/'))	*strchr(session_name,'/')=0;

	if (strstr(options, "psw="))
	{
		const char* PSW = strstr(options, "psw=") + 4;
		if (strchr(PSW, '/')) 
			strncpy_s(password_str, PSW, strchr(PSW, '/') - PSW);
		else
			strncpy_s(password_str, PSW, 63);
	}
	if (strstr(options, "maxplayers="))
	{
		const char* sMaxPlayers = strstr(options, "maxplayers=") + 11;
		string64 tmpStr = "";
		if (strchr(sMaxPlayers, '/')) 
			strncpy_s(tmpStr, sMaxPlayers, strchr(sMaxPlayers, '/') - sMaxPlayers);
		else
			strncpy_s(tmpStr, sMaxPlayers, 63);
		dwMaxPlayers = atol(tmpStr);
	}
	if (dwMaxPlayers > 32 || dwMaxPlayers<1) dwMaxPlayers = 32;
#ifdef DEBUG
	Msg("MaxPlayers = %d", dwMaxPlayers);
#endif // #ifdef DEBUG

	//-------------------------------------------------------------------
	BOOL bPortWasSet = FALSE;
	u32 dwServerPort = START_PORT_LAN_SV;
	if (strstr(options, "portsv="))
	{
		const char* ServerPort = strstr(options, "portsv=") + 7;
		string64 tmpStr = "";
		if (strchr(ServerPort, '/')) 
			strncpy_s(tmpStr, ServerPort, strchr(ServerPort, '/') - ServerPort);
		else
			strncpy_s(tmpStr, ServerPort, 63);
		dwServerPort = atol(tmpStr);
		clamp(dwServerPort, u32(START_PORT), u32(END_PORT));
		bPortWasSet = TRUE; //this is not casual game
	}
	//-------------------------------------------------------------------

#ifdef DEBUG
	string1024 tmp;
#endif // DEBUG
    // Create the IDirectPlay8Client object.
	HRESULT CoCreateInstanceRes = CoCreateInstance	(CLSID_DirectPlay8Server, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Server, (LPVOID*) &NET);
	//---------------------------	
	if (CoCreateInstanceRes != S_OK)
	{
		DXTRACE_ERR(tmp, CoCreateInstanceRes );
		CHK_DX(CoCreateInstanceRes );
	}	
	//---------------------------
	
    // Initialize IDirectPlay8Client object.
#ifdef DEBUG
    CHK_DX(NET->Initialize	(this, Handler, 0));
#else
	CHK_DX(NET->Initialize	(this, Handler, DPNINITIALIZE_DISABLEPARAMVAL));
#endif

	BOOL	bSimulator		= FALSE;
	if (strstr(Core.Params,"-netsim"))		bSimulator = TRUE;
	
	
	// dump_URL		("! sv ",	net_Address_device);

	// Set server-player info
    DPN_APPLICATION_DESC		dpAppDesc;
    DPN_PLAYER_INFO				dpPlayerInfo;
    WCHAR						wszName		[] = L"XRay Server";
	
    ZeroMemory					(&dpPlayerInfo, sizeof(DPN_PLAYER_INFO));
    dpPlayerInfo.dwSize			= sizeof(DPN_PLAYER_INFO);
    dpPlayerInfo.dwInfoFlags	= DPNINFO_NAME;
    dpPlayerInfo.pwszName		= wszName;
    dpPlayerInfo.pvData			= NULL;
    dpPlayerInfo.dwDataSize		= NULL;
    dpPlayerInfo.dwPlayerFlags	= 0;
	
	CHK_DX(NET->SetServerInfo( &dpPlayerInfo, NULL, NULL, DPNSETSERVERINFO_SYNC ) );
	
    // Set server/session description
	WCHAR	SessionNameUNICODE[4096];
	CHK_DX(MultiByteToWideChar(CP_ACP, 0, session_name, -1, SessionNameUNICODE, 4096 ));
    // Set server/session description
	
    // Now set up the Application Description
    ZeroMemory					(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
    dpAppDesc.dwSize			= sizeof(DPN_APPLICATION_DESC);
    dpAppDesc.dwFlags			= DPNSESSION_CLIENT_SERVER | DPNSESSION_NODPNSVR;
    dpAppDesc.guidApplication	= NET_GUID;
    dpAppDesc.pwszSessionName	= SessionNameUNICODE;
//	dpAppDesc.dwMaxPlayers		= (m_bDedicated) ? (dwMaxPlayers+2) : (dwMaxPlayers+1);
	dpAppDesc.dwMaxPlayers		= (dwMaxPlayers+2);
	dpAppDesc.pvApplicationReservedData	= &game_descr;
	dpAppDesc.dwApplicationReservedDataSize = sizeof(game_descr);

	WCHAR	SessionPasswordUNICODE[4096];
	if (xr_strlen(password_str))
	{
		CHK_DX(MultiByteToWideChar(CP_ACP, 0, password_str, -1, SessionPasswordUNICODE, 4096 ));
		dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
		dpAppDesc.pwszPassword = SessionPasswordUNICODE;
	};


	// Create our IDirectPlay8Address Device Address, --- Set the SP for our Device Address
	net_Address_device = NULL;
	CHK_DX(CoCreateInstance	(CLSID_DirectPlay8Address,NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address,(LPVOID*) &net_Address_device )); 
	CHK_DX(net_Address_device->SetSP		(bSimulator? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP ));
	
	DWORD dwTraversalMode = DPNA_TRAVERSALMODE_NONE;
	CHK_DX(net_Address_device->AddComponent(DPNA_KEY_TRAVERSALMODE, &dwTraversalMode, sizeof(dwTraversalMode), DPNA_DATATYPE_DWORD));

	HRESULT HostSuccess = S_FALSE;
	// We are now ready to host the app and will try different ports
	psNET_Port = dwServerPort;
	while (HostSuccess != S_OK)
	{
		CHK_DX(net_Address_device->AddComponent	(DPNA_KEY_PORT, &psNET_Port, sizeof(psNET_Port), DPNA_DATATYPE_DWORD ));

		HostSuccess = NET->Host			
			(
			&dpAppDesc,				// AppDesc
			&net_Address_device, 1, // Device Address
			NULL, NULL,             // Reserved
			NULL,                   // Player Context
			0 );					// dwFlags
		if (HostSuccess != S_OK)
		{
//			xr_string res = Debug.error2string(HostSuccess);
				if (bPortWasSet) 
				{
					Msg("! IPureServer : port %d is BUSY!", psNET_Port);
					return ErrConnect;
				}
				else
				{
					Msg("! IPureServer : port %d is BUSY!", psNET_Port);
				}

				psNET_Port++;
				if (psNET_Port > END_PORT_LAN)
				{
					return ErrConnect;
				}
		}
		else
		{
			Msg("- IPureServer : created on port %d!", psNET_Port);
		}
	};
	
	CHK_DX(HostSuccess);

	return	ErrNoError;
}

void IPureServer::Disconnect	()
{
    if( NET )	
		NET->Close(0);
	
	// Release interfaces
    _RELEASE	(net_Address_device);
    _RELEASE	(NET);
}
