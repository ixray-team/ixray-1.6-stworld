////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_creature_abstract.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife creature abstract class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Monsters.h"

void CSE_ALifeCreatureAbstract::on_spawn	()
{
	m_dynamic_out_restrictions.clear	();
	m_dynamic_in_restrictions.clear		();

	if (!g_Alive())
		m_game_death_time				= 0;
}


