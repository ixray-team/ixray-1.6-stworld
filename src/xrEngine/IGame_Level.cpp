#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "x_ray.h"
#include "std_classes.h"
#include "customHUD.h"
#include "render.h"
#include "gamefont.h"
#include "xrLevel.h"
#include "CameraManager.h"
#include "xr_object.h"
#include "feel_sound.h"
#include "Environment.h"

ENGINE_API	IGame_Level*	g_pGameLevel	= NULL;
extern	BOOL g_bLoaded;

IGame_Level::IGame_Level( )
:m_pCameras(NULL)
{
#ifndef DEDICATED_SERVER	
	m_pCameras					= xr_new<CCameraManager>(true);
#endif
	g_pGameLevel				= this;
	pLevel						= NULL;
	bReady						= false;
	pCurrentActor				= NULL;
	pCurrentViewActor			= NULL;
	Device.DumpResourcesMemoryUsage();
}

IGame_Level::~IGame_Level	()
{
	xr_delete					( pLevel );

	// Render-level unload
	Render->level_Unload		();
	xr_delete					(m_pCameras);
	// Unregister
	Device.seqRender.Remove		(this);
	Device.seqFrame.Remove		(this);

#ifndef DEDICATED_SERVER
	CCameraManager::ResetPP		();
#endif //#ifndef DEDICATED_SERVER

	Sound->set_geometry_occ		(NULL);
	Sound->set_handler			(NULL);
	Device.DumpResourcesMemoryUsage();

	u32		m_base=0,c_base=0,m_lmaps=0,c_lmaps=0;
	if (Device.m_pRender) 
		Device.m_pRender->ResourcesGetMemoryUsage(m_base,c_base,m_lmaps,c_lmaps);

	Msg		("* [ D3D ]: textures[%d K]", (m_base+m_lmaps)/1024);

}

void IGame_Level::net_Stop( )
{
	for (int i=0; i<6; i++)
		Objects.Update			(false);
	// Destroy all objects
	Objects.Unload				( );

#ifndef DEDICATED_SERVER
	IR_Release					( );
#endif

	bReady						= false;	
}

//-------------------------------------------------------------------------------------------
void __stdcall _sound_event		(ref_sound_data_ptr S, float range)
{
	if ( g_pGameLevel && S && S->feedback )	g_pGameLevel->SoundEvent_Register	(S,range);
}
static void __stdcall	build_callback	(Fvector* V, int Vcnt, CDB::TRI* T, int Tcnt, void* params)
{
	g_pGameLevel->Load_GameSpecific_CFORM( T, Tcnt );
}

BOOL IGame_Level::Load(u32 dwNum) 
{
	// Initialize level data
	pApp->Level_Set				( dwNum );
	string_path					temp;
	if (!FS.exist(temp, "$level$", "level.ltx"))
		Debug.fatal	(DEBUG_INFO,"Can't find level configuration file '%s'.",temp);
	pLevel						= xr_new<CInifile>	( temp );
	
	// Open
	g_pGamePersistent->LoadTitle	();
	IReader* LL_Stream			= FS.r_open	("$level$","level");
	IReader	&fs					= *LL_Stream;

	// Header
	hdrLEVEL					H;
	fs.r_chunk_safe				(fsL_HEADER,&H,sizeof(H));
	R_ASSERT2					(XRCL_PRODUCTION_VERSION==H.XRLC_version,"Incompatible level version.");

	// CForms
	g_pGamePersistent->LoadTitle();
	ObjectSpace.Load			( build_callback );

#ifndef DEDICATED_SERVER
	Sound->set_geometry_occ		(ObjectSpace.GetStaticModel	());
	Sound->set_handler			( _sound_event );
#endif //#ifndef DEDICATED_SERVER

	pApp->LoadSwitch			();


#ifndef DEDICATED_SERVER
	if(!g_hud)
		g_hud					= (CCustomHUD*)NEW_INSTANCE	(CLSID_HUDMANAGER);
#endif //#ifndef DEDICATED_SERVER

	Render->level_Load			(LL_Stream);

#ifndef DEDICATED_SERVER
	g_pGamePersistent->Environment().mods_load	();
#endif //#ifndef DEDICATED_SERVER

	g_pGamePersistent->LoadTitle();

	Objects.Load				();

	// Done
	FS.r_close					( LL_Stream );
	bReady						= true;

#ifndef DEDICATED_SERVER
	IR_Capture();
	Device.seqRender.Add		(this);
#endif

	Device.seqFrame.Add			(this);

	return TRUE;	
}

int		psNET_DedicatedSleep	= 5;

void IGame_Level::OnRender( ) 
{
#ifndef DEDICATED_SERVER
//	if (_abs(Device.fTimeDelta)<EPS_S) return;

	// Level render, only when no client output required
	if (!g_dedicated_server)	{
		Render->Calculate			();
		Render->Render				();
	} else {
		Sleep						(psNET_DedicatedSleep);
	}
#endif
}

void IGame_Level::OnFrame( ) 
{

	// Update all objects
	VERIFY						(bReady);
	Objects.Update				(false);

#ifndef DEDICATED_SERVER
	g_hud->OnFrame				();

	// Ambience
	if (Sounds_Random.size() && (Device.dwTimeGlobal > Sounds_Random_dwNextTime))
	{
		Sounds_Random_dwNextTime		= Device.dwTimeGlobal + ::Random.randI	(10000,20000);
		Fvector	pos;
		pos.random_dir().normalize().mul(::Random.randF(30,100)).add	(Device.vCameraPosition);
		int		id						= ::Random.randI(Sounds_Random.size());
		if (Sounds_Random_Enabled)		{
			Sounds_Random[id].play_at_pos	(0,pos,0);
			Sounds_Random[id].set_volume	(1.f);
			Sounds_Random[id].set_range		(10,200);
		}
	}
#endif //#ifndef DEDICATED_SERVER
}
// ==================================================================================================


void IGame_Level::SetEntity( CObject* O  )
{
	if (pCurrentActor)
		pCurrentActor->On_LostEntity();
	
	if (O)
		O->On_SetEntity();

	pCurrentActor=pCurrentViewActor=O;
}

void IGame_Level::SetViewEntity( CObject* O  )
{
	if (pCurrentViewActor)
		pCurrentViewActor->On_LostEntity();

	if (O)
		O->On_SetEntity();

	pCurrentViewActor=O;
}

void	IGame_Level::SoundEvent_Register	( ref_sound_data_ptr S, float range )
{
	if (!g_bLoaded)									return;
	if (!S)											return;
	if (S->g_object && S->g_object->getDestroy())	{S->g_object=0; return;}
	if (0==S->feedback)								return;

	clamp					(range,0.1f,500.f);

	const CSound_params* p	= S->feedback->get_params();
	Fvector snd_position	= p->position;
	if(S->feedback->is_2D()){
		snd_position.add	(Sound->listener_position());
	}

	VERIFY					(p && _valid(range) );
	range					= _min(range,p->max_ai_distance);
	VERIFY					(_valid(snd_position));
	VERIFY					(_valid(p->max_ai_distance));
	VERIFY					(_valid(p->volume));

	// Query objects
	Fvector					bb_size	=	{range,range,range};
	g_SpatialSpace->q_box	(snd_ER,0,STYPE_REACTTOSOUND,snd_position,bb_size);

	// Iterate
	xr_vector<ISpatial*>::iterator	it	= snd_ER.begin	();
	xr_vector<ISpatial*>::iterator	end	= snd_ER.end	();
	for (; it!=end; it++)	{
		Feel::Sound* L		= (*it)->dcast_FeelSound	();
		if (0==L)			continue;
		CObject* CO = (*it)->dcast_CObject();	VERIFY(CO);
		if (CO->getDestroy()) continue;

		// Energy and signal
		VERIFY				(_valid((*it)->spatial.sphere.P));
		float dist			= snd_position.distance_to((*it)->spatial.sphere.P);
		if (dist>p->max_ai_distance) continue;
		VERIFY				(_valid(dist));
		VERIFY2				(!fis_zero(p->max_ai_distance), S->handle->file_name());
		float Power			= (1.f-dist/p->max_ai_distance)*p->volume;
		VERIFY				(_valid(Power));
		if (Power>EPS_S)	{
			float occ		= Sound->get_occlusion_to((*it)->spatial.sphere.P,snd_position);
			VERIFY			(_valid(occ))	;
			Power			*= occ;
			if (Power>EPS_S)	{
				_esound_delegate	D	=	{ L, S, Power };
				snd_Events.push_back	(D)	;
			}
		}
	}
	snd_ER.clear_not_free	();
}

void	IGame_Level::SoundEvent_Dispatch	( )
{
	while	(!snd_Events.empty())	{
		_esound_delegate&	D	= snd_Events.back	();
		VERIFY				(D.dest && D.source);
		if (D.source->feedback)	{
			D.dest->feel_sound_new	(
				D.source->g_object,
				D.source->g_type,
				D.source->g_userdata,

				D.source->feedback->is_2D() ? Device.vCameraPosition : 
					D.source->feedback->get_params()->position,
				D.power
				);
		}
		snd_Events.pop_back		();
	}
}

// Lain: added
void   IGame_Level::SoundEvent_OnDestDestroy (Feel::Sound* obj)
{
	struct rem_pred
	{
		rem_pred(Feel::Sound* obj) : m_obj(obj) {}

		bool operator () (const _esound_delegate& d)
		{
			return d.dest == m_obj;
		}

	private:
		Feel::Sound* m_obj;
	};

	snd_Events.erase( std::remove_if(snd_Events.begin(), snd_Events.end(), rem_pred(obj)),
	                  snd_Events.end() );
}


