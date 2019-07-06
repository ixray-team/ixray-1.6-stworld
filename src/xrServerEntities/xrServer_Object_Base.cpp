////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Object_Base.cpp
//	Created 	: 19.09.2002
//  Modified 	: 16.07.2004
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server base object
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects.h"
#include "xrMessages.h"
#include "game_base_space.h"
#include "clsid_game.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

#ifndef AI_COMPILER
#	include "object_factory.h"
#endif

#ifndef XRSE_FACTORY_EXPORTS
#	include "xrEProps.h"
	
	IPropHelper &PHelper()
	{
		NODEFAULT;
#	ifdef DEBUG
		return(*(IPropHelper*)0);
#	endif
	}
#endif

////////////////////////////////////////////////////////////////////////////
// CSE_Abstract
////////////////////////////////////////////////////////////////////////////
CSE_Abstract::CSE_Abstract(LPCSTR caSection)
{
	m_editor_flags.zero			();
	RespawnTime					= 0;
	net_Ready					= FALSE;
	ID							= 0xffff;
	ID_Parent					= 0xffff;
	owner						= 0;
	m_gameType.SetDefaults		();
	s_RP						= 0xFE;			// Use supplied coords
	s_flags.assign				(0);
	s_name						= caSection;
	s_name_replace				= 0;			//xr_strdup("");
	o_Angle.set					(0.f,0.f,0.f);
	o_Position.set				(0.f,0.f,0.f);
//	m_bALifeControl				= false;
	m_wVersion					= 0;
	m_tClassID					= TEXT2CLSID(pSettings->r_string(caSection,"class"));

	//m_spawn_flags.zero			();
	//m_spawn_flags.set			(flSpawnEnabled			,TRUE);
	//m_spawn_flags.set			(flSpawnOnSurgeOnly		,TRUE);
	//m_spawn_flags.set			(flSpawnSingleItemOnly	,TRUE);
	//m_spawn_flags.set			(flSpawnIfDestroyedOnly	,TRUE);
	//m_spawn_flags.set			(flSpawnInfiniteCount	,TRUE);
	//m_ini_file					= 0;

	//if (pSettings->line_exist(caSection,"custom_data")) 
	//{
	//	pcstr const raw_file_name	= pSettings->r_string(caSection,"custom_data");
	//	IReader const* config	= 0;
	//	{
	//		string_path			file_name;
	//		FS.update_path		(file_name,"$game_config$", raw_file_name);
	//		if ( FS.exist(file_name) )
	//			config			= FS.r_open(file_name);
	//	}

	//	if ( config ) 
	//	{
	//		int					size = config->length()*sizeof(char);
	//		LPSTR				temp = (LPSTR)_alloca(size + 1);
	//		CopyMemory			(temp,config->pointer(),size);
	//		temp[size]			= 0;
	//		m_ini_string		= temp;

	//	{
	//		IReader* _r	= (IReader*)config;
	//		FS.r_close(_r);
	//	}

	//	}
	//	else
	//		Msg					( "! cannot open config file %s", raw_file_name );
	//}

}

CSE_Abstract::~CSE_Abstract					()
{
	xr_free						(s_name_replace);
//	xr_delete					(m_ini_file);
}

CSE_Visual* CSE_Abstract::visual			()
{
	return						(0);
}

ISE_Shape*  CSE_Abstract::shape				()
{
	return						(0);
}

CSE_Motion* CSE_Abstract::motion			()
{
	return						(0);
}

//CInifile &CSE_Abstract::spawn_ini			()
//{
//	if (!m_ini_file) 
//#pragma warning(push)
//#pragma warning(disable:4238)
//		m_ini_file			= xr_new<CInifile>(
//			&IReader			(
//				(void*)(*(m_ini_string)),
//				m_ini_string.size()
//			),
//			FS.get_path("$game_config$")->m_Path
//		);
//#pragma warning(pop)
//	return						(*m_ini_file);
//}

static enum EGameTypes {
	GAME_ANY							= 0,
	GAME_SINGLE							= 1,
	GAME_DEATHMATCH						= 2,
//	GAME_CTF							= 3,
//	GAME_ASSAULT						= 4,	// Team1 - assaulting, Team0 - Defending
	GAME_CS								= 5,
	GAME_TEAMDEATHMATCH					= 6,
	GAME_ARTEFACTHUNT					= 7,
	GAME_CAPTURETHEARTEFACT				= 8,

	//identifiers in range [100...254] are registered for script game type
	GAME_DUMMY							= 255	// temporary game type
};

void CSE_Abstract::Spawn_Write(NET_Packet	&tNetPacket, BOOL bLocal)
{
	tNetPacket.w_begin			(M_SPAWN);
	tNetPacket.w_u8				(SPAWN_VERSION);

	tNetPacket.w_stringZ		(s_name			);
	tNetPacket.w_stringZ		(s_name_replace ?	s_name_replace : "");
	tNetPacket.w_u8				(s_RP			);
	tNetPacket.w_vec3			(o_Position		);
	tNetPacket.w_vec3			(o_Angle		);
	tNetPacket.w_u16			(RespawnTime	);
	tNetPacket.w_u16			(ID				);
	tNetPacket.w_u16			(ID_Parent		);

	if (bLocal)
		tNetPacket.w_u16		(u16(s_flags.flags|M_SPAWN_OBJECT_LOCAL) );
	else
		tNetPacket.w_u16		(u16(s_flags.flags&~(M_SPAWN_OBJECT_LOCAL|M_SPAWN_OBJECT_ASPLAYER)));
	
	tNetPacket.w_u16			(m_gameType.m_GameType.get());

	// write specific data
	u32	position				= tNetPacket.w_tell();
	tNetPacket.w_u16			(0);
	STATE_Write					(tNetPacket);
	u16 size					= u16(tNetPacket.w_tell() - position);
//#ifdef XRSE_FACTORY_EXPORTS
	R_ASSERT3					((m_tClassID == CLSID_SPECTATOR) || (size > sizeof(size)),
		"object isn't successfully saved, get your backup :(",name_replace());
//#endif
	tNetPacket.w_seek			(position, &size, sizeof(u16));
}

BOOL CSE_Abstract::Spawn_Read( NET_Packet& tNetPacket )
{
	u16							dummy16;
	tNetPacket.r_begin			(dummy16);	
	R_ASSERT					(M_SPAWN==dummy16);
	tNetPacket.r_u8				(m_wVersion);

	tNetPacket.r_stringZ		(s_name);
	
	string256					temp;
	tNetPacket.r_stringZ		(temp);
	set_name_replace			(temp);

	tNetPacket.r_u8				(s_RP			);
	tNetPacket.r_vec3			(o_Position		);
	tNetPacket.r_vec3			(o_Angle		);
	tNetPacket.r_u16			(RespawnTime	);
	tNetPacket.r_u16			(ID				);
	tNetPacket.r_u16			(ID_Parent		);
	
	tNetPacket.r_u16			(s_flags.flags	); 
	
	u16 gt;
	tNetPacket.r_u16			(gt);
	m_gameType.m_GameType.assign(gt);

	//client object custom data serialization LOAD
	if(m_wVersion==1)
	{
		u16 client_data_size	= tNetPacket.r_u16();
		R_ASSERT(client_data_size==0);

		for(u16 i=0; i<client_data_size;++i)
			tNetPacket.r_u8();
	}

	u16							size;
	tNetPacket.r_u16			(size);	// size
	bool b1						= (m_tClassID == CLSID_SPECTATOR);
	bool b2						= (size > sizeof(size)) || (tNetPacket.inistream!=NULL);
	R_ASSERT3					( (b1 || b2),"cannot read object, which is not successfully saved :(",name_replace());
	STATE_Read					(tNetPacket,size);
	return						TRUE;
}

CSE_Abstract *CSE_Abstract::base	()
{
	return						(this);
}

const CSE_Abstract *CSE_Abstract::base	() const
{
	return						(this);
}

CSE_Abstract *CSE_Abstract::init	()
{
	return						(this);
}

LPCSTR		CSE_Abstract::name			() const
{
	return	(*s_name);
}

LPCSTR		CSE_Abstract::name_replace	() const
{
	return	(s_name_replace);
}

Fvector&	CSE_Abstract::position		()
{
	return	(o_Position);
}

Fvector&	CSE_Abstract::angle			()
{
	return	(o_Angle);
}

Flags16&	CSE_Abstract::flags			()
{
	return	(s_flags);
}

xr_token game_types[]={
	{ "any_game",				eGameIDNoGame				},
	{ "deathmatch",				eGameIDDeathmatch			},
	{ "team_deathmatch",		eGameIDTeamDeathmatch		},
	{ "artefacthunt",			eGameIDArtefactHunt			},
	{ "capture_the_artefact",	eGameIDCaptureTheArtefact	},
	{ 0,				0				}
};

#ifndef XRGAME_EXPORTS
void CSE_Abstract::FillProps(LPCSTR pref, PropItemVec& items)
{
#ifdef XRSE_FACTORY_EXPORTS
    m_gameType.FillProp(pref, items);
#endif // #ifdef XRSE_FACTORY_EXPORTS
/*
#ifdef XRGAME_EXPORTS
#	ifdef DEBUG
	PHelper().CreateToken8		(items,	PrepareKey(pref,"Game Type"),			&s_gameid,		game_types);
    PHelper().CreateU16			(items,	PrepareKey(pref, "Respawn Time (s)"),	&RespawnTime,	0,43200);

*/
}

void CSE_Abstract::FillProp					(LPCSTR pref, PropItemVec &items)
{
	FillProps					(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

bool CSE_Abstract::validate					()
{
	return						(true);
}
