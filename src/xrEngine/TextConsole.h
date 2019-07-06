////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_XRENGINE_TEXTCONSOLE_H_INCLUDED
#define ENGINE_XRENGINE_TEXTCONSOLE_H_INCLUDED

#include "xr_ioconsole.h"
#include "xrServerInfo.h"

class CTextConsole : public CConsole
{
private:
	typedef CConsole inherited;

private:
	u32			m_prev_log_count;
	HANDLE		m_console_output;
	HANDLE		m_console_input;

	SMALL_RECT	m_window_rect;

	CServerInfo m_server_info;

	void		UpdateLog			( );
	void		UpdateEditorStr		( );
	void		WriteLine			( const void* buffer, DWORD size, COORD position, u32 color = 0 );

public:
					CTextConsole	( );
	virtual			~CTextConsole	( );

	virtual	void	Initialize		( );
	virtual	void	Destroy			( );

	virtual void _BCL	OnFrame		( );
	virtual void xr_stdcall	Execute_cmd	( );
};// class CTextConsole

#endif // #ifndef ENGINE_XRENGINE_TEXTCONSOLE_H_INCLUDED