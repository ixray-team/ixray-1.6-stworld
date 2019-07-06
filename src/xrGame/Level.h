#if !defined(AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_)
#define AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_
#pragma once

#include "../xrEngine/IGame_Persistent.h"
#include "../xrNetServer/net_client.h"
#include "../xrEngine/StatGraph.h"
#include "xrMessages.h"
#include "alife_space.h"
#include "../xrcore/xrDebug.h"
#include "xrServer.h"
#include "GlobalFeelTouch.hpp"

#include "Level_network_map_sync.h"
#include "secure_messaging.h"
#include "traffic_optimization.h"


class	CHUDManager;
class	CParticlesObject;
class	xrServer;
class	game_cl_GameState;
class	NET_Queue_Event;
class	CSE_Abstract;
class	CGameObject;
class	CPHCommander;
class	CLevelSoundManager;
class	CZoneList;
class	message_filter;
class	CServerInfo;
class CBulletManager;
class CMapManager;

#ifdef DEBUG
	class	CDebugRenderer;
#endif

extern float g_fov;

const int maxRP					= 64;
const int maxTeams				= 32;

#define GP_UNIQUENICK_LEN           21


namespace file_transfer
{
	class client_site;
}; //namespace file_transfer

class CLevel :	public IGame_Level, 
				public IPureClient
{
	typedef IGame_Level		inherited;

	void					ClearAllObjects			();
protected:
	
	CLevelSoundManager*		m_level_sound_manager;
#ifdef DEBUG
	// debug renderer
	CDebugRenderer*			m_debug_renderer;
#endif

	CPHCommander*			m_ph_commander;
	CPHCommander*			m_ph_commander_physics_worldstep;
	// Local events
	EVENT					eChangeTrack;
	EVENT					eEnvironment;
	EVENT					eEntitySpawn;
	//---------------------------------------------
	CStatGraph*				pStatGraphS;
	u32						m_dwSPC;	//SendedPacketsCount
	u32						m_dwSPS;	//SendedPacketsSize
	CStatGraph*				pStatGraphR;
	u32						m_dwRPC;	//ReceivedPacketsCount
	u32						m_dwRPS;	//ReceivedPacketsSize
	//---------------------------------------------
	
public:
	////////////// network ////////////////////////
	u32						GetInterpolationSteps	();
	void					SetInterpolationSteps	(u32 InterpSteps);
	bool					InterpolationDisabled	();
	void					ReculcInterpolationSteps();
	u32						GetNumCrSteps			() const	{return m_dwNumSteps; };
	void					SetNumCrSteps			( u32 NumSteps );
	static void 			PhisStepsCallback		( u32 Time0, u32 Time1 );
	bool					In_NetCorrectionPrediction	() {return m_bIn_CrPr;};

	virtual void			OnInvalidHost			();
	virtual void			OnInvalidPassword		();
	virtual void			OnSessionFull			();
	virtual void			OnConnectRejected		();

private:
			
			void			OnSecureMessage			(NET_Packet & P);
			void			OnSecureKeySync			(NET_Packet & P);
			void			SecureSend				(NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED/*, u32 dwTimeout=0*/);
			
	secure_messaging::key_t	m_secret_key;
private:
	BOOL					m_bNeed_CrPr;
	u32						m_dwNumSteps;
	bool					m_bIn_CrPr;

	DEF_VECTOR				(OBJECTS_LIST, CGameObject*);

	OBJECTS_LIST			pObjects4CrPr;
	OBJECTS_LIST			pActors4CrPr;

	CObject*				pCurrentControlEntity;
public:
	void					AddObject_To_Objects4CrPr	(CGameObject* pObj);
	void					AddActor_To_Actors4CrPr		(CGameObject* pActor);

	void					RemoveObject_From_4CrPr		(CGameObject* pObj);	

	CObject*				CurrentControlEntity	( void ) const		{ return pCurrentControlEntity; }
	void					SetControlEntity		( CObject* O  )		{ pCurrentControlEntity=O; }
private:
	
	void					make_NetCorrectionPrediction	();

	u32						m_dwDeltaUpdate ;
	u32						m_dwLastNetUpdateTime;

	void					UpdateDeltaUpd					( u32 LastTime );
	BOOL					Connect2Server					(LPCSTR options);

public:
	shared_str const			get_cdkey_digest() const { return m_variables.m_client_digest; };
	void						OnConnectResult					(NET_Packet* P);
	
	DEFINE_VECTOR				(CParticlesObject*,POVec,POIt);
	POVec						m_StaticParticles;

	game_cl_GameState*			game;
	NET_Queue_Event*			game_events;
	xr_deque<CSE_Abstract*>		game_spawn_queue;
	xrServer*					Server;
	GlobalFeelTouch				m_feel_deny;
	
	CZoneList*					hud_zones_list;
	CZoneList*					create_hud_zones_list();

	struct{
		BOOL			net_start_result_total;
		BOOL			connected_to_server;
		BOOL			deny_m_spawn;//only for debug...
		BOOL			sended_request_connection_data;
		BOOL			m_game_config_started;
		BOOL			m_game_configured;
		bool			m_bConnectResultReceived;
		bool			m_bConnectResult;
		xr_string		m_sConnectResult;
	xrServer::EConnect	m_connect_server_err;
	shared_str			m_client_digest;	//for screenshots

	u32		m_dwCL_PingDeltaSend;
	u32		m_dwCL_PingLastSendTime;
	u32		m_dwRealPing;

	#ifdef DEBUG
		bool		m_bSynchronization;
		bool		m_bEnvPaused;
	#endif
	
	} m_variables;

protected:

		
	void						MakeReconnect();
	
	LevelMapSyncData			map_data;
	bool						synchronize_map_data		();
	bool						synchronize_client			();

	bool	xr_stdcall			net_start_server1			();
	bool	xr_stdcall			net_start_server2			();
	bool	xr_stdcall			net_start_server3			();
	bool	xr_stdcall			net_start_server4			();
	bool	xr_stdcall			net_start_server5			();
	bool	xr_stdcall			net_start_server6			();

	bool	xr_stdcall			net_start_client1			();
	bool	xr_stdcall			net_start_client2			();
	bool	xr_stdcall			net_start_client3			();
	bool	xr_stdcall			net_start_client4			();
	bool	xr_stdcall			net_start_client5			();
	bool	xr_stdcall			net_start_client6			();

	virtual BOOL				LoadLobbyMenu			( LPCSTR lobby_menu_name );

	void						CalculateLevelCrc32		();
public:
	bool						IsChecksumsEqual		(u32 check_sum) const;

	// sounds
	xr_vector<ref_sound*>		static_Sounds;

	// startup options
	shared_str					m_caServerOptions;
	shared_str					m_caClientOptions;

	// Starting/Loading
	virtual BOOL				net_Start				( LPCSTR op_server, LPCSTR op_client);
	virtual void				net_Stop				( );
	virtual void				net_Update				( );


	virtual BOOL				Load_GameSpecific_After ( );
	virtual void				Load_GameSpecific_CFORM	( CDB::TRI* T, u32 count );

	// Events
	virtual void				OnEvent					( EVENT E, u64 P1, u64 P2 );
	virtual void	_BCL		OnFrame					( void );
	virtual void				OnRender				( );

	void						cl_Process_Event		(u16 dest, u16 type, NET_Packet& P);
	void						cl_Process_Spawn		(NET_Packet& P);
	void						ProcessGameEvents		( );
	void						ProcessGameSpawns		( );
	void						ProcessCompressedUpdate	(NET_Packet& P, u8 const compression_type);

	// Input
	virtual	void				IR_OnKeyboardPress		( int btn );
	virtual void				IR_OnKeyboardRelease	( int btn );
	virtual void				IR_OnKeyboardHold		( int btn );
	virtual void				IR_OnMousePress			( int btn );
	virtual void				IR_OnMouseRelease		( int btn );
	virtual void				IR_OnMouseHold			( int btn );
	virtual void				IR_OnMouseMove			( int, int);
	virtual void				IR_OnMouseStop			( int, int);
	virtual void				IR_OnMouseWheel			( int direction);
	virtual void				IR_OnActivate			(void);
	
			int					get_RPID				(LPCSTR name);


	// Game
	void						InitializeClientGame	(NET_Packet& P);
	void						ClientReceive			();
	void						ClientSend				();
	void						ClientSendProfileData	();
	virtual	void				Send					(NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED/*, u32 dwTimeout=0*/);
	
	void						g_cl_Spawn				(LPCSTR name, u8 rp, u16 flags, Fvector pos);	// only ask server
	void						g_sv_Spawn				(CSE_Abstract* E);					// server reply/command spawning
	
	void						SLS_Default				();					// Default/Editor Load
	
#ifdef DEBUG
	IC CDebugRenderer&			debug_renderer					( );
#endif

	IC CPHCommander&			ph_commander					( );
	IC CPHCommander&			ph_commander_physics_worldstep	( );

	// C/D
	CLevel();
	virtual ~CLevel();

	virtual shared_str			map_name				() const { return map_data.m_name; }
	//		shared_str			version					() const { return map_data.m_map_version.c_str(); } //this method can be used ONLY from CCC_ChangeGameType

	virtual void				GetLevelInfo		( CServerInfo* si );

	//gets the time from the game simulation
	//возвращает время в милисекундах относительно начала игры
	ALife::_TIME_ID		GetStartGameTime		();
	ALife::_TIME_ID		GetGameTime				();
	//возвращает время для энвайронмента в милисекундах относительно начала игры
	ALife::_TIME_ID		GetEnvironmentGameTime	();
	//игровое время в отформатированном виде
	void				GetGameDateTime			(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs);

	float				GetGameTimeFactor		();
	void				SetGameTimeFactor		(const float fTimeFactor);
	void				SetGameTimeFactor		(ALife::_TIME_ID GameTime, const float fTimeFactor);
	virtual void		SetEnvironmentGameTimeFactor(u64 const& GameTime, float const& fTimeFactor);

	// gets current daytime [0..23]
	u8					GetDayTime				();
	u32					GetGameDayTimeMS		();
	float				GetGameDayTimeSec		();
	float				GetEnvironmentGameDayTimeSec();

protected:	
	CMapManager*			m_map_manager;

public:
	CMapManager&			MapManager					() const 	{return *m_map_manager;}
protected:	
	CBulletManager*		m_pBulletManager;
public:
	IC CBulletManager&	BulletManager() {return	*m_pBulletManager;}

	//by Mad Max 
			bool			IsServer					( );
			bool			IsPureClient				( );
			CSE_Abstract*	spawn_item					( LPCSTR section, const Fvector &position, u32 level_vertex_id, u16 parent_id, bool return_item = false );
			
public:
			void			remove_objects				();
			virtual void	OnSessionTerminate		(LPCSTR reason);
			
			file_transfer::client_site*					m_file_transfer;
			
	compression::ppmd_trained_stream*			m_trained_stream;
	compression::lzo_dictionary_buffer			m_lzo_dictionary;
	//alligned to 16 bytes m_lzo_working_buffer
	u8*											m_lzo_working_memory;
	u8*											m_lzo_working_buffer;
	
	void			init_compression			();
	void			deinit_compression			();
};

IC CLevel&				Level()		{ return *((CLevel*) g_pGameLevel);			}
IC game_cl_GameState&	Game()		{ return *Level().game;					}
u32						GameID();

#ifdef DEBUG
IC CDebugRenderer& CLevel::debug_renderer()
{
	VERIFY				(m_debug_renderer);
	return				(*m_debug_renderer);
}
#endif

IC CPHCommander& CLevel::ph_commander()
{
	VERIFY(m_ph_commander);
	return *m_ph_commander;
}

IC CPHCommander& CLevel::ph_commander_physics_worldstep()
{
	VERIFY(m_ph_commander_physics_worldstep);
	return *m_ph_commander_physics_worldstep;
}

IC bool		OnServer()	{ return Level().IsServer();}
IC bool		OnClient()	{ return Level().IsPureClient();}

extern BOOL				g_bDebugEvents;

#endif // !defined(AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_)