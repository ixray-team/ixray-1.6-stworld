////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_anomalous_zone.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife anomalous zone class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Monsters.h"


void CSE_ALifeAnomalousZone::on_spawn						()
{
//	inherited::on_spawn		();
//	spawn_artefacts			();
}

bool CSE_ALifeAnomalousZone::keep_saved_data_anyway() const
{
	return (true);
}

