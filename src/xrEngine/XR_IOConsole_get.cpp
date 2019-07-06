////////////////////////////////////////////////////////////////////////////
//	Module 		: XR_IOConsole_get.cpp
//	Created 	: 17.05.2008
//	Author		: Evgeniy Sokolov
//	Description : Console`s get-functions class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"


bool command_storage::GetBool( LPCSTR cmd ) const
{
	IConsole_Command* cc	= GetCommand(cmd);
	CCC_Mask* cf			= dynamic_cast<CCC_Mask*>(cc);
	if ( cf )
	{
		return ( cf->GetValue() != 0 );
	}

	CCC_Integer* ci			= dynamic_cast<CCC_Integer*>(cc);
	if ( ci )
	{
		return ( ci->GetValue() != 0 );
	}
	return false;
}

float command_storage::GetFloat( LPCSTR cmd, float& min, float& max ) const
{
	min						= 0.0f;
	max						= 0.0f;
	IConsole_Command* cc	= GetCommand(cmd);
	CCC_Float* cf			= dynamic_cast<CCC_Float*>(cc);
	if ( cf )
	{
		cf->GetBounds(min, max);
		return cf->GetValue(); 
	}
	return 0.0f;
}

int command_storage::GetInteger( LPCSTR cmd, int& min, int& max ) const
{
	min						= 0;
	max						= 1;
	IConsole_Command* cc	= GetCommand(cmd);

	CCC_Integer* cf			= dynamic_cast<CCC_Integer*>(cc);
	if ( cf )
	{
		cf->GetBounds(min, max);
		return cf->GetValue();
	}
	CCC_Mask* cm = dynamic_cast<CCC_Mask*>(cc);
	if ( cm )
	{
		min = 0;
		max = 1;
		return ( cm->GetValue() )? 1 : 0;
	}
	return 0;
}

LPCSTR command_storage::GetString( LPCSTR cmd ) const
{
	IConsole_Command* cc	= GetCommand(cmd);
	if(!cc)
		return				NULL;

	static IConsole_Command::TStatus stat;
	cc->Status				( stat );
	return					stat;
}

LPCSTR command_storage::GetToken( LPCSTR cmd ) const
{
	return GetString( cmd );
}

xr_token* command_storage::GetXRToken( LPCSTR cmd ) const
{
	IConsole_Command* cc	= GetCommand(cmd);
	
	CCC_Token* cf			= dynamic_cast<CCC_Token*>(cc);
	if ( cf )
	{
		return cf->GetToken();
	}
	return					NULL;
}

Fvector* command_storage::GetFVectorPtr( LPCSTR cmd ) const
{
	IConsole_Command* cc	= GetCommand(cmd);
	CCC_Vector3* cf			= dynamic_cast<CCC_Vector3*>(cc);
	if ( cf )
	{
		return cf->GetValuePtr();
	}
	return					NULL;
}

Fvector command_storage::GetFVector( LPCSTR cmd ) const
{
	Fvector* pV = GetFVectorPtr( cmd );
	if ( pV )
	{
		return *pV;
	}
	return Fvector().set( 0.0f, 0.0f, 0.0f );
}
