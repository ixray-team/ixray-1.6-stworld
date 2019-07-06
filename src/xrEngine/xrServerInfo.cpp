////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServerInfo.h"

void CServerInfo::AddItem( LPCSTR name_, LPCSTR value_, u32 color_ )
{
	shared_str s_name( name_ );
	AddItem( s_name, value_, color_ );
}

void CServerInfo::AddItem( shared_str& name_, LPCSTR value_, u32 color_ )
{
	SItem_ServerInfo it;
	//	shared_str s_name = CStringTable().translate( name_ );

	//	xr_strcpy( it.name, s_name.c_str() );
	xr_strcpy( it.name, name_.c_str() );
	xr_strcat( it.name, " = " );
	xr_strcat( it.name, value_ );
	it.color = color_;

	if ( data.size() < max_item )
	{
		data.push_back( it );
	}
}
