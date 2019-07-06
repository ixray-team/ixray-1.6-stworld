#include "stdafx.h"
#include "NET_Common.h"
#include "net_server.h"
#include <functional>

#include "NET_Log.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

static	INetLog* pSvNetLog = NULL; 

#define NET_NOTFOR_SUBNET_STR		  "Your IP does not present in server's subnet"

void	dump_URL	(LPCSTR p, IDirectPlay8Address* A);

LPCSTR nameTraffic	= "traffic.net";

XRNETSERVER_API int		psNET_ServerUpdate	= 30;		// FPS
XRNETSERVER_API int		psNET_ServerPending	= 3;

XRNETSERVER_API ClientID BroadcastCID(0xffffffff);



IClient::IClient( CTimer* timer )
:net_Statistic(timer),
server(NULL)
{
	dwTime_LastUpdate	= 0;
	flags.bLocal = FALSE;
	flags.bConnected = FALSE;
	flags.bReconnect = FALSE;
}

IClient::~IClient()
{
}

void IClientStatistic::Update(DPN_CONNECTION_INFO& CI)
{
	u32 time_global		= TimeGlobal(device_timer);
	if (time_global-dwBaseTime >= 999)
	{
		dwBaseTime		= time_global;
		
		mps_recive		= CI.dwMessagesReceived - mps_receive_base;
		mps_receive_base= CI.dwMessagesReceived;

		u32	cur_msend	= CI.dwMessagesTransmittedHighPriority+CI.dwMessagesTransmittedNormalPriority+CI.dwMessagesTransmittedLowPriority;
		mps_send		= cur_msend - mps_send_base;
		mps_send_base	= cur_msend;

		dwBytesSendedPerSec		= dwBytesSended;
		dwBytesSended			= 0;
		dwBytesReceivedPerSec	= dwBytesReceived;
		dwBytesReceived			= 0;
	}
	ci_last	= CI;
}


//------------------------------------------------------------------------------

void IClient::_SendTo_LL( const void* data, u32 size, u32 flags )
{
    R_ASSERT(server);
    server->IPureServer::SendTo_LL( ID, const_cast<void*>(data), size, flags );
}

IClient* IPureServer::ID_to_client(ClientID ID, bool ScanAll)
{
	if ( 0 == ID.value() )			return NULL;
	IClient* ret_client = GetClientByID(ID);
	if (ret_client || !ScanAll)
		return ret_client;
	
	return NULL;
}

void IPureServer::_Recieve( const void* data, u32 data_size, u32 param )
{
	if (data_size >= NET_PacketSizeLimit) {
		Msg		("! too large packet size[%d] received, DoS attack?", data_size);
		return;
	}

    NET_Packet  packet; 
    ClientID    id;

    id.set( param );
    packet.construct( data, data_size );
	//DWORD currentThreadId = GetCurrentThreadId();
	//Msg("-S- Entering to csMessages from _Receive [%d]", currentThreadId);
	csMessage.Enter();
	//LogStackTrace(
	//		make_string("-S- Entered to csMessages [%d]", currentThreadId).c_str());
	//---------------------------------------
	
	OnDataReceived( packet );

	//---------------------------------------
	u32	result = OnMessage( packet, id );
	//Msg("-S- Leaving from csMessages [%d]", currentThreadId);
	csMessage.Leave();
	
	if( result )		
	    SendBroadcast( id, packet, result );
}

//==============================================================================

IPureServer::IPureServer	(CTimer* timer)
#ifdef PROFILE_CRITICAL_SECTIONS
:csPlayers(MUTEX_PROFILE_ID(IPureServer::csPlayers))
,csMessage(MUTEX_PROFILE_ID(IPureServer::csMessage))
#endif // PROFILE_CRITICAL_SECTIONS
{
	device_timer			= timer;
	stats.clear				();
	stats.dwSendTime		= TimeGlobal(device_timer);
	SV_Client				= NULL;
	NET						= NULL;
	net_Address_device		= NULL;
	pSvNetLog				= NULL;//xr_new<INetLog>("logs\\net_sv_log.log", TimeGlobal(device_timer));
#ifdef DEBUG
	sender_functor_invoked = false;
#endif
}

IPureServer::~IPureServer( )
{
	SV_Client				= NULL;
	xr_delete				(pSvNetLog); 
}

void	IPureServer::Flush_Clients_Buffers( )
{
    #if NET_LOG_PACKETS
    Msg( "#flush server send-buf" );
    #endif
	
	struct LocalSenderFunctor
	{
		static void FlushBuffer(IClient* client)
		{
			client->MultipacketSender::FlushSendBuffer(0);
		}
	};
	
	net_players.ForEachClientDo(
		LocalSenderFunctor::FlushBuffer
	);
}

void	IPureServer::SendTo_Buf(ClientID id, const void* data, u32 size, u32 dwFlags )
{
	IClient* tmp_client = net_players.GetFoundClient(
		ClientIdSearchPredicate(id));
	VERIFY(tmp_client);
	tmp_client->MultipacketSender::SendPacket(data, size, dwFlags );
}


void IPureServer::OnDataSent( const void* data, u32 size )
{
// for logging purpose
	if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS))
	{
		if (!pSvNetLog) pSvNetLog = xr_new<INetLog>("logs\\net_sv_log.log", TimeGlobal(device_timer));
		pSvNetLog->LogData(TimeGlobal(device_timer), data, size);
	}

#ifdef _DEBUG
	u32 time_global		= TimeGlobal(device_timer);
	if (time_global - stats.dwSendTime >= 999)
	{
		stats.dwBytesPerSec = (stats.dwBytesPerSec*9 + stats.dwBytesSended)/10;
		stats.dwBytesSended = 0;
		stats.dwSendTime = time_global;
	};
	if ( ID.value() )
		stats.dwBytesSended += size;
#endif
}

void IPureServer::OnDataReceived( NET_Packet const& packet )
{
	if( psNET_Flags.test(NETFLAG_LOG_SV_PACKETS) ) 
	{
		if( !pSvNetLog) pSvNetLog = xr_new<INetLog>("logs\\net_sv_log.log", TimeGlobal(device_timer));
	    pSvNetLog->LogPacket( TimeGlobal(device_timer), &packet, TRUE );
	}
}

void IPureServer::SendTo(ClientID ID/*DPNID ID*/, NET_Packet& P, u32 dwFlags )
{
	SendTo_LL( ID, P.B.data, P.B.count, dwFlags );
}

void IPureServer::SendBroadcast_LL(ClientID exclude, const void* data, u32 size, u32 dwFlags)
{
	struct ClientExcluderPredicate
	{
		ClientID id_to_exclude;
		ClientExcluderPredicate(ClientID exclude) :
			id_to_exclude(exclude)
		{}
		bool operator()(IClient* client)
		{
			if (client->ID == id_to_exclude)
				return false;
			if (!client->flags.bConnected)
				return false;
			return true;
		}
	};
	struct ClientSenderFunctor
	{
		IPureServer*	m_owner;
		const void*			m_data;
		u32				m_size;
		u32				m_dwFlags;
		ClientSenderFunctor(IPureServer* owner, const void* data, u32 size, u32 dwFlags) :
			m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags)
		{}
		void operator()(IClient* client)
		{
			m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags);			
		}
	};
	ClientSenderFunctor temp_functor(this, data, size, dwFlags);
	net_players.ForFoundClientsDo(ClientExcluderPredicate(exclude), temp_functor);
}

void	IPureServer::SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags)
{
	// Perform broadcasting
	SendBroadcast_LL( exclude, P.B.data, P.B.count, dwFlags );
}

u32	IPureServer::OnMessage	(NET_Packet& P, ClientID sender)	// Non-Zero means broadcasting with "flags" as returned
{
	/*
	u16 m_type;
	P.r_begin	(m_type);
	switch (m_type)
	{
	case M_CHAT:
		{
			char	buffer[256];
			P.r_string(buffer);
			printf	("RECEIVE: %s\n",buffer);
		}
		break;
	}
	*/
	
	return 0;
}

void IPureServer::OnCL_Connected		(IClient* CL)
{
	Msg("* Player 0x%08x connected.\n",	CL->ID.value());
}
void IPureServer::OnCL_Disconnected		(IClient* CL)
{
	Msg("* Player 0x%08x disconnected.\n", CL->ID.value());
}

BOOL IPureServer::HasBandwidth(IClient* C)
{
	u32	dwTime			= TimeGlobal(device_timer);
	u32	dwInterval		= 0;

	if (psNET_ServerUpdate != 0) 
		dwInterval = 1000/psNET_ServerUpdate; 

	if(psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))	
		dwInterval	= 1000;	// approx 2 times per second

	HRESULT hr;
	if(psNET_ServerUpdate != 0 && (dwTime-C->dwTime_LastUpdate)>dwInterval)
	{
		// check queue for "empty" state
		DWORD				dwPending;
		hr					= NET->GetSendQueueInfo(C->ID.value(),&dwPending,0,0);
		if (FAILED(hr))		return FALSE;

		if (dwPending > u32(psNET_ServerPending))	
		{
			C->net_Statistic.dwTimesBlocked++;
			return FALSE;
		};

		UpdateClientStatistic(C);
		// ok
		C->dwTime_LastUpdate	= dwTime;
		return TRUE;
	}
	return FALSE;
}

void IPureServer::UpdateClientStatistic(IClient* C)
{
	// Query network statistic for this client
	DPN_CONNECTION_INFO			CI;
	ZeroMemory					(&CI,sizeof(CI));
	CI.dwSize					= sizeof(CI);

	HRESULT hr					= NET->GetConnectionInfo(C->ID.value(), &CI, 0);
	if (FAILED(hr))				
		return;

	C->net_Statistic.Update		(CI);
}

void IPureServer::ClearStatistic	()
{
	stats.clear();
	struct StatsClearFunctor
	{
		static void Clear(IClient* client)
		{
			client->net_Statistic.Clear();
		}
	};
	net_players.ForEachClientDo(StatsClearFunctor::Clear);
};

/*bool			IPureServer::DisconnectClient	(IClient* C)
{
	if (!C) return false;

	string64 Reason = "st_kicked_by_server";
	HRESULT res = NET->DestroyClient(C->ID.value(), Reason, xr_strlen(Reason)+1, 0);
	CHK_DX(res);
	return true;
}*/

bool IPureServer::DisconnectClient(IClient* C, LPCSTR Reason)
{
	if (!C) return false;

	HRESULT res = NET->DestroyClient(C->ID.value(), Reason, xr_strlen(Reason)+1, 0);
	CHK_DX(res);
	return true;
}


bool IPureServer::GetClientAddress(IDirectPlay8Address* pClientAddress, ip_address& Address, DWORD* pPort)
{
	WCHAR				wstrHostname[ 256 ] = {0};
	DWORD dwSize		= sizeof(wstrHostname);
	DWORD dwDataType	= 0;
	CHK_DX(pClientAddress->GetComponentByName( DPNA_KEY_HOSTNAME, wstrHostname, &dwSize, &dwDataType ));

	string256				HostName;
	CHK_DX(WideCharToMultiByte(CP_ACP,0,wstrHostname,-1,HostName,sizeof(HostName),0,0));

	Address.set		(HostName);

	if (pPort != NULL)
	{
		DWORD dwPort			= 0;
		DWORD dwPortSize		= sizeof(dwPort);
		DWORD dwPortDataType	= DPNA_DATATYPE_DWORD;
		CHK_DX(pClientAddress->GetComponentByName( DPNA_KEY_PORT, &dwPort, &dwPortSize, &dwPortDataType ));
		*pPort					= dwPort;
	};

	return true;
};

bool IPureServer::GetClientAddress	(ClientID ID, ip_address& Address, DWORD* pPort)
{
	IDirectPlay8Address* pClAddr	= NULL;
	CHK_DX(NET->GetClientAddress	(ID.value(), &pClAddr, 0));

	return GetClientAddress			(pClAddr, Address, pPort);
};

