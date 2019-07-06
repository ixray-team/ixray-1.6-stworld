// XR_IOConsole.h: interface for the CConsole class.
//
//////////////////////////////////////////////////////////////////////
#ifndef XR_IOCONSOLE_H_INCLUDED
#define XR_IOCONSOLE_H_INCLUDED

#include "xrServerInfo.h"

class ENGINE_API IConsole_Command;

#define ioc_prompt ">>> "
#define ch_cursor "_"

namespace text_editor
{
class line_editor_base;
class line_edit_control;
};

struct TipString
{
	shared_str	text;
	int			HL_start; // Highlight
	int			HL_finish;
	
	TipString()
	{
		text._set( "" );
		HL_start = 0;
		HL_finish = 0;
	}
	TipString( shared_str const& tips_text, int start_pos, int finish_pos )
	{
		text._set( tips_text );
		HL_start = start_pos;
		HL_finish = finish_pos;
	}
	TipString( LPCSTR tips_text, int start_pos, int finish_pos )
	{
		text._set( tips_text );
		HL_start = start_pos;
		HL_finish = finish_pos;
	}
	TipString( shared_str const& tips_text )
	{
		text._set( tips_text );
		HL_start = 0;
		HL_finish = 0;
	}
	IC bool operator== ( shared_str const& tips_text )
	{
		return ( text == tips_text );
	}
};


class ENGINE_API CConsole :
	public pureFrame
{
public:
	typedef  xr_vector<shared_str>						vecHistory;
	typedef  xr_vector<shared_str>						vecTips;
	typedef  xr_vector<TipString>						vecTipsEx;

	enum			{ CONSOLE_BUF_SIZE = 1024 };
	enum			{ VIEW_TIPS_COUNT = 14, MAX_TIPS_COUNT = 220 };

protected:
	int				scroll_delta;


	POINT			m_mouse_pos;
	bool			m_disable_tips;

	vecHistory		m_cmd_history;
	u32				m_cmd_history_max;
	int				m_cmd_history_idx;
	shared_str		m_last_cmd;

	vecTips			m_temp_tips;
	vecTipsEx		m_tips;
	u32				m_tips_mode;
	shared_str		m_cur_cmd;
	int				m_select_tip;
	int				m_start_tip;
	u32				m_prev_length_str;

	u32				m_last_time;
	CServerInfo		m_server_info;

public:
						CConsole				( );
	virtual				~CConsole				( );
	virtual	void		Initialize				( );
	virtual void		Destroy					( );

	virtual void _BCL	OnFrame					( );
	bool				bVisible;

	virtual void		Show				( );
	virtual void		Hide				( );

	void				ExecuteString		( LPCSTR cmd, bool record_cmd );
	virtual void xr_stdcall Execute_cmd		( );
	void				SelectCommand		( );

protected:
	text_editor::line_editor_base*			m_editor;
	text_editor::line_edit_control&		ec();

	enum Console_mark // (int)=char
	{
		no_mark = ' ',
		mark0  = '~',
		mark1  = '!', // error
		mark2  = '@', // console cmd
		mark3  = '#',
		mark4  = '$',
		mark5  = '%',
		mark6  = '^',
		mark7  = '&',
		mark8  = '*',
		mark9  = '-', // green = ok
		mark10 = '+',
		mark11 = '=',
		mark12 = '/'
	};
	
	bool	is_mark				( Console_mark type );
	u32		get_mark_color		( Console_mark type );

	void	Register_callbacks	();
	
protected:
	void xr_stdcall Prev_log	();
	void xr_stdcall Next_log	();
	void xr_stdcall Begin_log	();
	void xr_stdcall End_log		();
	
	void xr_stdcall Find_cmd	();
	void xr_stdcall Find_cmd_back();
	void xr_stdcall Prev_cmd	();
	void xr_stdcall Next_cmd	();
	void xr_stdcall Prev_tip	();
	void xr_stdcall Next_tip	();
	
	void xr_stdcall Begin_tips	();
	void xr_stdcall End_tips	();
	void xr_stdcall PageUp_tips	();
	void xr_stdcall PageDown_tips();

	void xr_stdcall Show_cmd	();
	void xr_stdcall Hide_cmd	();
	void xr_stdcall Hide_cmd_esc();

protected:
	void	add_cmd_history		( shared_str const& str );
	void	next_cmd_history_idx();
	void	prev_cmd_history_idx();
	void	reset_cmd_history_idx();

	void	next_selected_tip	();
	void	check_next_selected_tip();
	void	prev_selected_tip	();
	void	check_prev_selected_tip();
	void	reset_selected_tip	();

	IConsole_Command* find_next_cmd	( LPCSTR in_str, shared_str& out_str );
	bool	add_next_cmds			( LPCSTR in_str, vecTipsEx& out_v );
	bool	add_internal_cmds		( LPCSTR in_str, vecTipsEx& out_v );
	
	void	update_tips				( );
	void	select_for_filter		( LPCSTR filter_str, vecTips& in_v, vecTipsEx& out_v );
}; // class CConsole


ENGINE_API extern CConsole* pConsole;

#endif // XR_IOCONSOLE_H_INCLUDED
