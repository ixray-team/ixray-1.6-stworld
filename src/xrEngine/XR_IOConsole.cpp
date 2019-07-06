// XR_IOConsole.cpp: implementation of the CConsole class.
// modify 15.05.2008 sea

#include "stdafx.h"
#include "XR_IOConsole.h"
#include "line_editor.h"

#include "x_ray.h"
#include "xr_ioc_cmd.h"

static u32   const cmd_history_max  = 64;
static u32 const default_font_color = color_rgba( 250, 250, 250, 250 );

ENGINE_API CConsole*		pConsole			=	NULL;
ENGINE_API command_storage* pConsoleCommands	=	NULL;


text_editor::line_edit_control& CConsole::ec()
{
	return m_editor->control();
}

u32 CConsole::get_mark_color( Console_mark type )
{
	u32 color = default_font_color;
	switch ( type )
	{
	case mark0:  color = color_rgba( 255, 255,   0, 255 ); break;
	case mark1:  color = color_rgba( 255,   0,   0, 255 ); break;
	case mark2:  color = color_rgba( 100, 100, 255, 255 ); break;
	case mark3:  color = color_rgba(   0, 222, 205, 155 ); break;
	case mark4:  color = color_rgba( 255,   0, 255, 255 ); break;
	case mark5:  color = color_rgba( 155,  55, 170, 155 ); break;
	case mark6:  color = color_rgba(  25, 200,  50, 255 ); break;
	case mark7:  color = color_rgba( 255, 255,   0, 255 ); break;
	case mark8:  color = color_rgba( 128, 128, 128, 255 ); break;
	case mark9:  color = color_rgba(   0, 255,   0, 255 ); break;
	case mark10: color = color_rgba(  55, 155, 140, 255 ); break;
	case mark11: color = color_rgba( 205, 205, 105, 255 ); break;
	case mark12: color = color_rgba( 128, 128, 250, 255 ); break;
	case no_mark:
	default: break;
	}
	return color;
}

bool CConsole::is_mark( Console_mark type )
{
	switch ( type )
	{
	case mark0:  case mark1:  case mark2:  case mark3:
	case mark4:  case mark5:  case mark6:  case mark7:
	case mark8:  case mark9:  case mark10: case mark11:	case mark12:
		return true;
		break;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CConsole::CConsole()
:m_editor(NULL)
{
	m_cmd_history_max = cmd_history_max;
	m_disable_tips    = false;
}

void CConsole::Initialize()
{
	Register_callbacks( );
	scroll_delta	= 0;
	bVisible		= false;

	m_mouse_pos.x	= 0;
	m_mouse_pos.y	= 0;
	m_last_cmd		= NULL;
	
	m_cmd_history.reserve( m_cmd_history_max + 2 );
	m_cmd_history.clear_not_free();
	reset_cmd_history_idx();

	m_tips.reserve( MAX_TIPS_COUNT + 1 );
	m_tips.clear_not_free();
	m_temp_tips.reserve( MAX_TIPS_COUNT + 1 );
	m_temp_tips.clear_not_free();

	m_tips_mode		= 0;
	m_prev_length_str = 0;
	m_cur_cmd		= NULL;
	reset_selected_tip();

	// Commands
	extern void CCC_Register();
	CCC_Register();

	m_last_time        = Device.dwTimeGlobal;
}

CConsole::~CConsole()
{
	xr_delete( m_editor );
	Destroy();
}

void CConsole::Destroy()
{
}

void CConsole::OnFrame()
{
	m_editor->on_frame();
	
	if ( Device.dwFrame % 10 == 0 )
	{
		update_tips();
	}
}


void CConsole::ExecuteString( LPCSTR cmd_str, bool record_cmd )
{
	bool result = pConsoleCommands->Execute		( cmd_str );

	if( result)
	{
		u32  str_size	= xr_strlen( cmd_str );
		PSTR edt		= (PSTR)_alloca( (str_size + 1) * sizeof(char) );
		
		xr_strcpy		( edt, str_size+1, cmd_str );
		edt[str_size]	= 0;

		scroll_delta			= 0;
		reset_cmd_history_idx	( );
		reset_selected_tip		( );

		text_editor::remove_spaces( edt );
		if ( edt[0] == 0 )
			return;

		if ( record_cmd )
		{
			char c[2];
			c[0] = mark2;
			c[1] = 0;

			if ( m_last_cmd.c_str() == 0 || xr_strcmp( m_last_cmd, edt ) != 0 )
			{
				Log				( c, edt );
				add_cmd_history	( edt );
				m_last_cmd		= edt;
			}
		}

		if ( record_cmd )
			ec().clear_states();
	}
}

void CConsole::Show()
{
	if ( bVisible )
	{
		return;
	}
	bVisible = true;
	
	GetCursorPos( &m_mouse_pos );

	ec().clear_states();
	scroll_delta	= 0;
	reset_cmd_history_idx();
	reset_selected_tip();
	update_tips();

	m_editor->CaptureInput();
	Device.seqFrame.Add( this );
}

void CConsole::Hide()
{
	if ( !bVisible )
		return;

	bVisible				= false;
	reset_selected_tip		( );
	update_tips				( );

	Device.seqFrame.Remove	(this );
	m_editor->ReleaseInput	( );
}

void CConsole::SelectCommand( )
{
	if ( m_cmd_history.empty() )
	{
		return;
	}
	VERIFY( 0 <= m_cmd_history_idx && m_cmd_history_idx < (int)m_cmd_history.size() );
		
	vecHistory::reverse_iterator	it_rb = m_cmd_history.rbegin() + m_cmd_history_idx;
	ec().set_edit( (*it_rb).c_str() );
	reset_selected_tip();
}

IConsole_Command* CConsole::find_next_cmd( LPCSTR in_str, shared_str& out_str )
{
	LPCSTR radmin_cmd_name = "ra ";
	bool b_ra  = (in_str == strstr( in_str, radmin_cmd_name ) );
	u32 offset = (b_ra)? xr_strlen( radmin_cmd_name ) : 0;

	LPSTR t2;
	STRCONCAT( t2, in_str + offset, " " );

	IConsole_Command* cc = pConsoleCommands->GetNextCommand( t2);
	if ( cc )
	{
		LPCSTR name_cmd      = cc->Name();
		u32    name_cmd_size = xr_strlen( name_cmd );
		PSTR   new_str       = (PSTR)_alloca( (offset + name_cmd_size + 2) * sizeof(char) );

		xr_strcpy( new_str, offset + name_cmd_size + 2, (b_ra)? radmin_cmd_name : "" );
		xr_strcat( new_str, offset + name_cmd_size + 2, name_cmd );

		out_str._set( (LPCSTR)new_str );
		return cc;
	}
	return NULL;
}

bool CConsole::add_next_cmds( LPCSTR in_str, vecTipsEx& out_v )
{
	u32 cur_count = out_v.size();
	if ( cur_count >= MAX_TIPS_COUNT )
	{
		return false;
	}

	LPSTR t2;
	STRCONCAT( t2, in_str, " " );

	shared_str temp;
	IConsole_Command* cc = find_next_cmd( t2, temp );
	if ( !cc || temp.size() == 0 )
	{
		return false;
	}

	bool res = false;
	for ( u32 i = cur_count; i < MAX_TIPS_COUNT*2; ++i ) //fake=protect
	{
		temp._set( cc->Name() );
		bool dup = ( std::find( out_v.begin(), out_v.end(), temp ) != out_v.end() );
		if ( !dup )
		{
			TipString ts( temp );
			out_v.push_back( ts );
			res = true;
		}
		if ( out_v.size() >= MAX_TIPS_COUNT )
		{
			break; // for
		}
		LPSTR t3;
		STRCONCAT( t3, out_v.back().text.c_str(), " " );
		cc = find_next_cmd( t3, temp );
		if ( !cc )
		{
			break; // for
		}
	} // for
	return res;
}

bool CConsole::add_internal_cmds( LPCSTR in_str, vecTipsEx& out_v )
{
	u32 cur_count = out_v.size();
	if ( cur_count >= MAX_TIPS_COUNT )
	{
		return false;
	}
	u32   in_sz = xr_strlen(in_str);
	
	bool res = false;
	// word in begin
	command_storage::vecCMD_IT itb = pConsoleCommands->m_commands.begin();
	command_storage::vecCMD_IT ite = pConsoleCommands->m_commands.end();

	for ( ; itb != ite; ++itb )
	{
		LPCSTR name		= itb->first;
		u32 name_sz		= xr_strlen(name);
		PSTR  name2		= (PSTR)_alloca( (name_sz+1) * sizeof(char) );
		
		if ( name_sz >= in_sz )
		{
			strncpy_s( name2, name_sz+1, name, in_sz );
			name2[in_sz] = 0;

			if ( !stricmp( name2, in_str ) )
			{
				shared_str temp = name;
				bool dup = ( std::find( out_v.begin(), out_v.end(), temp ) != out_v.end() );
				if ( !dup )
				{
					out_v.push_back( TipString( temp, 0, in_sz ) );
					res = true;
				}
			}
		}

		if ( out_v.size() >= MAX_TIPS_COUNT )
			return res;
	} // for

	// word in internal
	itb = pConsoleCommands->m_commands.begin();
	ite = pConsoleCommands->m_commands.end();
	for ( ; itb != ite; ++itb )
	{
		LPCSTR name = itb->first;
		LPCSTR fd_str = strstr( name, in_str );
		if ( fd_str )
		{
			shared_str temp;
			temp._set( name );
			bool dup = ( std::find( out_v.begin(), out_v.end(), temp ) != out_v.end() );
			if ( !dup )
			{
				u32 name_sz = xr_strlen( name );
				int   fd_sz = name_sz - xr_strlen( fd_str );
				out_v.push_back( TipString( temp, fd_sz, fd_sz + in_sz ) );
				res = true;
			}
		}
		if ( out_v.size() >= MAX_TIPS_COUNT )
		{
			return res;
		}
	} // for

	return res;
}

void CConsole::update_tips()
{
	m_temp_tips.clear_not_free();
	m_tips.clear_not_free();

	m_cur_cmd  = NULL;
	if ( !bVisible )
	{
		return;
	}

	LPCSTR cur = ec().str_edit();
	u32    cur_length = xr_strlen( cur );

	if ( cur_length == 0 )
	{
		m_prev_length_str = 0;
		return;
	}
	
	if ( m_prev_length_str != cur_length )
	{
		reset_selected_tip();
	}
	m_prev_length_str = cur_length;

	PSTR first = (PSTR)_alloca( (cur_length + 1) * sizeof(char) );
	PSTR last  = (PSTR)_alloca( (cur_length + 1) * sizeof(char) );
	text_editor::split_cmd( first, last, cur );
	
	u32 first_lenght = xr_strlen(first);
	
	if ( (first_lenght > 2) && (first_lenght + 1 <= cur_length) ) // param
	{
		if ( cur[first_lenght] == ' ' )
		{
			if ( m_tips_mode != 2 )
			{
				reset_selected_tip();
			}

			IConsole_Command* cc = pConsoleCommands->GetCommand( first );
			if ( cc )
			{
				u32 mode = 0;
				if ( (first_lenght + 2 <= cur_length) && (cur[first_lenght] == ' ') && (cur[first_lenght+1] == ' ') )
				{
					mode = 1;
					last += 1; // fake: next char
				}

				cc->fill_tips( m_temp_tips, mode );
				m_tips_mode = 2;
				m_cur_cmd._set( first );
				select_for_filter( last, m_temp_tips, m_tips );

				if ( m_tips.size() == 0 )
				{
					m_tips.push_back( TipString( "(empty)" ) );
				}
				if ( (int)m_tips.size() <= m_select_tip )
				{
					reset_selected_tip();
				}
				return;
			}
		}
	}

	add_internal_cmds( cur, m_tips );
	m_tips_mode = 1;

	if ( m_tips.size() == 0 )
	{
		m_tips_mode = 0;
		reset_selected_tip();
	}

	if ( (int)m_tips.size() <= m_select_tip )
		reset_selected_tip();
}

void CConsole::select_for_filter( LPCSTR filter_str, vecTips& in_v, vecTipsEx& out_v )
{
	out_v.clear_not_free();
	u32 in_count = in_v.size();
	if ( in_count == 0 || !filter_str )
	{
		return;
	}

	bool all = ( xr_strlen(filter_str) == 0 );

	vecTips::iterator itb = in_v.begin();
	vecTips::iterator ite = in_v.end();
	for ( ; itb != ite ; ++itb )
	{
		shared_str const& str = (*itb);
		if ( all )
		{
			out_v.push_back( TipString( str ) );
		}else
		{
			LPCSTR fd_str = strstr( str.c_str(), filter_str );
			if( fd_str )
			{
				int   fd_sz = str.size() - xr_strlen( fd_str );
				TipString ts( str, fd_sz, fd_sz + xr_strlen(filter_str) );
				out_v.push_back( ts );
			}
		}
	}//for
}
