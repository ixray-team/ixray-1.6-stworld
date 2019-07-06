////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TextConsole.h"
#include "line_editor.h"
#include "xr_ioc_cmd.h"
#include "igame_level.h"

BOOL WINAPI CtrlHandlerRoutine( DWORD CtrlType )
{
	switch (CtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			{
			pConsoleCommands->Execute("quit");
			}	
	}
	return TRUE;
}

CTextConsole::CTextConsole( )
:m_prev_log_count(0)
{
	m_editor          = xr_new<text_editor::line_editor_console>( (u32)CONSOLE_BUF_SIZE );
}

CTextConsole::~CTextConsole( )
{}

void CTextConsole::Initialize( )
{
	inherited::Initialize();
	BOOL res			= AllocConsole();

	m_console_output	= GetStdHandle(STD_OUTPUT_HANDLE);
	m_console_input		= GetStdHandle(STD_INPUT_HANDLE);

	res = SetConsoleCtrlHandler( CtrlHandlerRoutine, TRUE );
	DWORD mode;
	res = GetConsoleMode(m_console_input, &mode);

	if(mode&ENABLE_PROCESSED_INPUT)
		Log("ENABLE_PROCESSED_INPUT");
	if(mode&ENABLE_LINE_INPUT)
		Log("ENABLE_LINE_INPUT");
	if(mode&ENABLE_ECHO_INPUT)
		Log("ENABLE_ECHO_INPUT");
	if(mode&ENABLE_WINDOW_INPUT)
		Log("ENABLE_WINDOW_INPUT");
	if(mode&ENABLE_MOUSE_INPUT)
		Log("ENABLE_MOUSE_INPUT");
	if(mode&ENABLE_INSERT_MODE)
		Log("ENABLE_INSERT_MODE");
	if(mode&ENABLE_QUICK_EDIT_MODE)
		Log("ENABLE_QUICK_EDIT_MODE");
	if(mode&ENABLE_EXTENDED_FLAGS)
		Log("ENABLE_EXTENDED_FLAGS");
	if(mode&ENABLE_AUTO_POSITION)
		Log("ENABLE_AUTO_POSITION");

	res = GetConsoleMode(m_console_output, &mode);
	if(mode&ENABLE_PROCESSED_OUTPUT)
		Log("ENABLE_PROCESSED_OUTPUT");
	if(mode&ENABLE_WRAP_AT_EOL_OUTPUT)
		Log("ENABLE_WRAP_AT_EOL_OUTPUT");



	CONSOLE_SCREEN_BUFFER_INFO scb_info;
	res = GetConsoleScreenBufferInfo(m_console_output, &scb_info);

	m_window_rect.Left = 0;
	m_window_rect.Top = 0;
	m_window_rect.Right = 79;
	m_window_rect.Bottom = 49;

	res = SetConsoleWindowInfo		(m_console_output, TRUE, &m_window_rect);

	COORD size={
		80,
		50
	};
	res = SetConsoleScreenBufferSize(m_console_output, size);
 
	CONSOLE_CURSOR_INFO cursor_info;
	cursor_info.bVisible	= TRUE;
	cursor_info.dwSize		= 20;

	res = SetConsoleCursorInfo(m_console_output, &cursor_info );
	((text_editor::line_editor_console*)m_editor)->set_input_handle(m_console_input);

	UpdateEditorStr( );
}

void CTextConsole::Destroy( )
{
	SetConsoleCtrlHandler( CtrlHandlerRoutine, FALSE );
	inherited::Destroy();
}

void CTextConsole::OnFrame( )
{
	inherited::OnFrame();

	if(LogFile->size()!=m_prev_log_count)
		UpdateLog();

	if(ec().need_update())
		UpdateEditorStr( );
}

void CTextConsole::UpdateLog( )
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo( m_console_output, &info );
	
//	SHORT server_info_size = 0;
// 	if ( g_pGameLevel && ( Device.dwTimeGlobal - m_last_time > 500 ) )
// 	{
// 		m_last_time = Device.dwTimeGlobal;
// 		
// 		m_server_info.ResetData();
// 		g_pGameLevel->GetLevelInfo( &m_server_info );
// 
// 		server_info_size = (SHORT)m_server_info.Size();
// 
// 		for( SHORT i=0; i<server_info_size; ++i )
// 		{
// 			COORD c_pos={0,i};
// 			WriteLine( m_server_info[i].name, xr_strlen(m_server_info[i].name), c_pos, 7 );				
// 		}
// 		COORD c_pos={0,server_info_size};
// 		FillConsoleOutputCharacter(m_console_output, ' ', m_window_rect.Right-m_window_rect.Left + 1, c_pos, 0 );
// 		UpdateEditorStr();
// 	}

	u32 log_count = LogFile->size();

	m_prev_log_count = log_count;

	// clear screen buffer
	DWORD written;
	COORD c={0,0};
	FillConsoleOutputCharacter( m_console_output, 
								' ', 
								(m_window_rect.Right-m_window_rect.Left+1)*(m_window_rect.Bottom-m_window_rect.Top), 
								c, 
								&written ); 

	//LPCSTR line1 = "Destroying object[319b590][319b590] [7][level_prefix_campfire_0000] frame[27]qwertyuiopasdfghjkl";
	//LPCSTR line2 = "object Destroying [4fdvhhv][dfgynn0] [4][level_prefix_campfire_8978] frame[32]asdfgjkklllzxcvvbnm";
	//COORD c_pos={0,0};
	//WriteLine( line1, xr_strlen(line1), c_pos, 0 );

	//COORD c_pos2={0,2};
	//WriteLine( line2, xr_strlen(line2), c_pos2, 0 );

#if (1)
	COORD c_pos = {0, m_window_rect.Bottom};
	int log_line_idx = log_count-1;

	while( c_pos.Y>=0 && log_line_idx>=0 )
	{
		shared_str& line	= (*LogFile)[log_line_idx];
		u32 line_length		= line.size();

		u32 needed_lines_count	= line_length/m_window_rect.Right;
		c_pos.Y					-= needed_lines_count+1;

		
 		u32 color = 0;
// 
// 		if ( line[0] == '~' )
// 			color = 0x0007;
// 		else if ( line[0] == '!' )
// 			color = FOREGROUND_RED;
// 		else if ( line[0] == '*' )
// 			color = FOREGROUND_BLUE;
// 		else if ( line[0] == '-' )
// 			color = 0x0006;
// 	
		WriteLine( line.c_str(), line.size(), c_pos, color );
		--log_line_idx;
	}
#endif

	SetConsoleCursorPosition(m_console_output, info.dwCursorPosition );
}

void CTextConsole::UpdateEditorStr( )
{
	COORD c_pos			= {0,m_window_rect.Bottom};
	SHORT prompt_len	= (SHORT)xr_strlen(ioc_prompt);

	DWORD written;
	FillConsoleOutputCharacter( m_console_output, ' ', m_window_rect.Right-m_window_rect.Left + 1, c_pos, &written ); 

	WriteLine			( ioc_prompt, prompt_len, c_pos, 0x0007 );

	LPCSTR edit_str		= ec().str_edit();
	SHORT edit_len		= (SHORT)xr_strlen(edit_str);
	c_pos.X				+= (SHORT)prompt_len;

	WriteLine			( edit_str, edit_len, c_pos, 0x0007 );

	c_pos.X				+= edit_len;
	SetConsoleCursorPosition(m_console_output, c_pos );
}

void CTextConsole::Execute_cmd( ) // DIK_RETURN, DIK_NUMPADENTER
{
	inherited::Execute_cmd( );
	ec().set_edit("");
}

void CTextConsole::WriteLine( const void* buffer, DWORD size, COORD position, u32 color )
{
	if (! SetConsoleTextAttribute(m_console_output, (WORD)color | FOREGROUND_INTENSITY))
	{
		MessageBox(NULL, TEXT("SetConsoleTextAttribute"), 
			TEXT("Console Error"), MB_OK);		
	}
	DWORD written;
	SetConsoleCursorPosition	( m_console_output, position );
	WriteConsole				( m_console_output,buffer, size, &written, NULL );
}
