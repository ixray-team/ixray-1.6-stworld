////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrMessages.h"
#include "xrServer_Object_Base.h"
#include "phnetstate.h"

#pragma warning(push)
#pragma warning(disable:4005)

//------------------------------------------------------------------------------
// Version history
//------------------------------------------------------------------------------
// 1  moved packet version to head, removed phantom and other old data
// 2 removed custom data, spawn flags, alife control
// 3 removed ini-string
// 4 removed CSE_ALifeObject data

#define SPAWN_VERSION	u8(4)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_Shape,ISE_Shape,CShapeData)
public:
	void							cform_read		(NET_Packet& P);
	void							cform_write		(NET_Packet& P);
									CSE_Shape		();
	virtual							~CSE_Shape		();
	virtual ISE_Shape*  __stdcall	shape			() = 0;
	virtual void __stdcall			assign_shapes	(CShapeData::shape_def* shapes, u32 cnt);
};

SERVER_ENTITY_DECLARE_BEGIN(CSE_Spectator,CSE_Abstract)
									CSE_Spectator	(LPCSTR caSection);
	virtual							~CSE_Spectator	();
	virtual u8						g_team			();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN(CSE_Temporary,CSE_Abstract)
	u32								m_tNodeID;
									CSE_Temporary	(LPCSTR caSection);
	virtual							~CSE_Temporary	();
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN0(CSE_PHSkeleton)
								CSE_PHSkeleton(LPCSTR caSection);
virtual							~CSE_PHSkeleton();

enum{
	flActive					= (1<<0),
	flSpawnCopy					= (1<<1),
	flSavedData					= (1<<2),
	flNotSave					= (1<<3)
};
	Flags8							_flags;
	SPHBonesData					saved_bones;
	u16								source_id;//for break only
	virtual bool					need_save				() const{return(!_flags.test(flNotSave));}
	virtual	void					set_sorce_id			(u16 si){source_id=si;}
	virtual u16						get_source_id			(){return source_id;}
	virtual CSE_Abstract			*cast_abstract			() {return 0;}
public:
SERVER_ENTITY_DECLARE_END

SERVER_ENTITY_DECLARE_BEGIN2(CSE_AbstractVisual,CSE_Abstract,CSE_Visual)
	typedef CSE_Abstract			inherited1;
	typedef CSE_Visual				inherited2;

	CSE_AbstractVisual										(LPCSTR caSection);
	virtual	~CSE_AbstractVisual								();
	virtual CSE_Visual* __stdcall	visual					();
	LPCSTR							getStartupAnimation		();
SERVER_ENTITY_DECLARE_END

#ifndef AI_COMPILER
extern CSE_Abstract	*F_entity_Create	(LPCSTR caSection);
#endif


#pragma warning(pop)
