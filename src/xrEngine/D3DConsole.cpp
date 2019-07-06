////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "D3DConsole.h"

#include "GameFont.h"
#include "../Include/xrRender/UIRender.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "line_editor.h"
#include "xr_input.h"

static float const UI_BASE_WIDTH	= 1024.0f;
static float const UI_BASE_HEIGHT	= 768.0f;
static float const LDIST            = 0.05f;
static u32 const prompt_font_color  = color_rgba( 228, 228, 255, 255 );
static u32 const tips_font_color    = color_rgba( 230, 250, 230, 255 );
static u32 const cmd_font_color     = color_rgba( 138, 138, 245, 255 );
static u32 const cursor_font_color  = color_rgba( 255, 255, 255, 255 );
static u32 const total_font_color   = color_rgba( 250, 250,  15, 180 );

static u32 const back_color         = color_rgba(  20,  20,  20, 200 );
static u32 const tips_back_color    = color_rgba(  20,  20,  20, 200 );
static u32 const tips_select_color  = color_rgba(  90,  90, 140, 230 );
static u32 const tips_word_color    = color_rgba(   5, 100,  56, 200 );
static u32 const tips_scroll_back_color  = color_rgba( 15, 15, 15, 230 );
static u32 const tips_scroll_pos_color   = color_rgba( 70, 70, 70, 240 );

CD3DConsole::CD3DConsole()
:m_hShader_back	(NULL),
pFont			(NULL),
pFont2			(NULL)
{
	m_editor          = xr_new<text_editor::line_editor_dinput>( (u32)CONSOLE_BUF_SIZE );
	Device.seqResolutionChanged.Add(this);
}

CD3DConsole::~CD3DConsole()
{
}

void CD3DConsole::Initialize( )
{
	inherited::Initialize();
}

void CD3DConsole::Destroy( )
{
	inherited::Destroy	( );
	xr_delete			( pFont );
	xr_delete			( pFont2 );
	xr_delete			( m_hShader_back );
	Device.seqResolutionChanged.Remove(this);
}

void CD3DConsole::OnScreenResolutionChanged()
{
	xr_delete( pFont );
	xr_delete( pFont2 );
}

void CD3DConsole::Show( )
{
	inherited::Show();
	Device.seqRender.Add( this, 1 );
}

void CD3DConsole::Hide( )
{
	inherited::Hide();

	if ( pInput->get_exclusive_mode() )
		SetCursorPos( m_mouse_pos.x, m_mouse_pos.y );

	Device.seqRender.Remove( this );
}

void CD3DConsole::OutFont( LPCSTR text, float& pos_y )
{
	float str_length	= pFont->SizeOf_( text );
	float scr_width		= 1.98f * Device.fWidth_2;

	if( str_length > scr_width ) //1024.0f
	{
		float f	= 0.0f;
		int sz	= 0;
		int ln	= 0;
		PSTR one_line = (PSTR)_alloca( (CONSOLE_BUF_SIZE + 1) * sizeof(char) );
		
		while( text[sz] && (ln + sz < CONSOLE_BUF_SIZE-5) )// перенос строк
		{
			one_line[ln+sz]   = text[sz];
			one_line[ln+sz+1] = 0;
			
			float t	= pFont->SizeOf_( one_line + ln );
			if ( t > scr_width )
			{
				OutFont		( text + sz + 1, pos_y );
				pos_y		-= LDIST;
				pFont->OutI	( -1.0f, pos_y, "%s", one_line + ln );
				ln			= sz + 1;
				f			= 0.0f;
			}
			else
			{
				f = t;
			}
			++sz;
		}
	}
	else
	{
		pFont->OutI( -1.0f, pos_y, "%s", text );
	}
}

void CD3DConsole::OnRender()
{
	if( !bVisible )
		return;

	if ( !m_hShader_back )
	{
		m_hShader_back = xr_new< FactoryPtr<IUIShader> >();
		(*m_hShader_back)->create( "hud\\default", "ui\\ui_console" ); // "ui\\ui_empty"
	}
	
	if ( !pFont )
	{
		pFont = xr_new<CGameFont>( "hud_font_di", CGameFont::fsDeviceIndependent );
		pFont->SetHeightI(  0.025f );
	}
	if( !pFont2 )
	{
		pFont2 = xr_new<CGameFont>( "hud_font_di2", CGameFont::fsDeviceIndependent );
		pFont2->SetHeightI( 0.025f );
	}

	bool bGame = false;	
	if ( ( g_pGameLevel && g_pGameLevel->bReady ) ||
		 ( g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive() ) )
	{
		 bGame = true;
	}
	
	DrawBackgrounds( bGame );

	float fMaxY;
	float dwMaxY = (float)Device.dwHeight;
	// float dwMaxX=float(Device.dwWidth/2);
	if ( bGame )
	{
		fMaxY  = 0.0f;
		dwMaxY /= 2;
	}
	else
	{
		fMaxY = 1.0f;
	}

	float ypos  = fMaxY - LDIST * 1.1f;
	float scr_x = 1.0f / Device.fWidth_2;

	//---------------------------------------------------------------------------------
	float scr_width  = 1.9f * Device.fWidth_2;
	float ioc_d      = pFont->SizeOf_(ioc_prompt);
	float d1         = pFont->SizeOf_( "_" );

	LPCSTR s_cursor = ec().str_before_cursor();
	LPCSTR s_b_mark = ec().str_before_mark();
	LPCSTR s_mark   = ec().str_mark();
	LPCSTR s_mark_a = ec().str_after_mark();

	//	strncpy_s( buf1, cur_pos, editor, MAX_LEN );
	float str_length = ioc_d + pFont->SizeOf_( s_cursor );
	float out_pos    = 0.0f;
	if( str_length > scr_width )
	{
		out_pos -= (str_length - scr_width);
		str_length = scr_width;
	}

	pFont->SetColor( prompt_font_color );
	pFont->OutI( -1.0f + out_pos * scr_x, ypos, "%s", ioc_prompt );
	out_pos += ioc_d;

	if ( !m_disable_tips && m_tips.size() )
	{
		pFont->SetColor( tips_font_color );

		float shift_x = 0.0f;
		switch ( m_tips_mode )
		{
		case 0: shift_x = scr_x * 1.0f;			break;
		case 1: shift_x = scr_x * out_pos;		break;
		case 2: shift_x = scr_x * ( ioc_d + pFont->SizeOf_(m_cur_cmd.c_str()) + d1 );	break;
		case 3: shift_x = scr_x * str_length;	break;
		}

		vecTipsEx::iterator itb = m_tips.begin() + m_start_tip;
		vecTipsEx::iterator ite = m_tips.end();
		for ( u32 i = 0; itb != ite ; ++itb, ++i ) // tips
		{
			pFont->OutI( -1.0f + shift_x, fMaxY + i*LDIST, "%s", (*itb).text.c_str() );
			if ( i >= VIEW_TIPS_COUNT-1 )
			{
				break; //for
			}
		}	
	}

	// ===== ==============================================
	pFont->SetColor ( cmd_font_color );
	pFont2->SetColor( cmd_font_color );

	pFont->OutI(  -1.0f + out_pos * scr_x, ypos, "%s", s_b_mark );		out_pos += pFont->SizeOf_(s_b_mark);
	pFont2->OutI( -1.0f + out_pos * scr_x, ypos, "%s", s_mark );		out_pos += pFont2->SizeOf_(s_mark);
	pFont->OutI(  -1.0f + out_pos * scr_x, ypos, "%s", s_mark_a );

	//pFont2->OutI( -1.0f + ioc_d * scr_x, ypos, "%s", editor=all );
	
	if( ec().cursor_view() )
	{
		pFont->SetColor( cursor_font_color );
		pFont->OutI( -1.0f + str_length * scr_x, ypos, "%s", ch_cursor );
	}
	
	// ---------------------
	u32 log_line = LogFile->size()-1;
	ypos -= LDIST;
	for( int i = log_line - scroll_delta; i >= 0; --i ) 
	{
		ypos -= LDIST;
		if ( ypos < -1.0f )
		{
			break;
		}
		LPCSTR ls = ((*LogFile)[i]).c_str();
		
		if ( !ls )
		{
			continue;
		}
		Console_mark cm = (Console_mark)ls[0];
		pFont->SetColor( get_mark_color( cm ) );
		//u8 b = (is_mark( cm ))? 2 : 0;
		//OutFont( ls + b, ypos );
		OutFont( ls, ypos );
	}
	
	string16 q;
	itoa( log_line, q, 10 );
	u32 qn = xr_strlen( q );
	pFont->SetColor( total_font_color );
	pFont->OutI( 0.95f - 0.03f * qn, fMaxY - 2.0f * LDIST, "[%d]", log_line );
		
	pFont->OnRender();
	pFont2->OnRender();
}

void CD3DConsole::DrawBackgrounds( bool bGame )
{
	float ky = (bGame)? 0.5f : 1.0f;

	Frect r;
	r.set( 0.0f, 0.0f, float(Device.dwWidth), ky * float(Device.dwHeight) );

	UIRender->SetShader( **m_hShader_back );
	// 6 = back, 12 = tips, (VIEW_TIPS_COUNT+1)*6 = highlight_words, 12 = scroll
	UIRender->StartPrimitive( 6 + 12 + (VIEW_TIPS_COUNT+1)*6 + 12, IUIRender::ptTriList, IUIRender::pttTL );

	DrawRect( r, back_color );

	if ( m_tips.size() == 0 || m_disable_tips )
	{
		UIRender->FlushPrimitive();
		return;
	}

	LPCSTR max_str = "xxxxx";
	vecTipsEx::iterator itb = m_tips.begin();
	vecTipsEx::iterator ite = m_tips.end();
	for ( ; itb != ite; ++itb )
	{
		if ( pFont->SizeOf_( (*itb).text.c_str() ) > pFont->SizeOf_( max_str ) )
		{
			max_str = (*itb).text.c_str();
		}
	}

	float w1        = pFont->SizeOf_( "_" );
	float ioc_w     = pFont->SizeOf_( ioc_prompt ) - w1;
	float cur_cmd_w = pFont->SizeOf_( m_cur_cmd.c_str() );
	cur_cmd_w		+= (cur_cmd_w > 0.01f) ? w1 : 0.0f;

	float list_w    = pFont->SizeOf_( max_str ) + 2.0f * w1;

	float font_h    = pFont->CurrentHeight_();
	float tips_h    = _min( m_tips.size(), (u32)VIEW_TIPS_COUNT ) * font_h;
	tips_h			+= ( m_tips.size() > 0 )? 5.0f : 0.0f;

	Frect pr, sr;
	pr.x1 = ioc_w + cur_cmd_w;
	pr.x2 = pr.x1 + list_w;

	pr.y1 = UI_BASE_HEIGHT * 0.5f;
	pr.y1 *= float(Device.dwHeight)/UI_BASE_HEIGHT;

	pr.y2 = pr.y1 + tips_h;

	float select_y = 0.0f;
	float select_h = 0.0f;
	
	if ( m_select_tip >= 0 && m_select_tip < (int)m_tips.size() )
	{
		int sel_pos = m_select_tip - m_start_tip;

		select_y = sel_pos * font_h;
		select_h = font_h; //1 string
	}
	
	sr.x1 = pr.x1;
	sr.y1 = pr.y1 + select_y;

	sr.x2 = pr.x2;
	sr.y2 = sr.y1 + select_h;

	DrawRect( pr, tips_back_color );
	DrawRect( sr, tips_select_color );

	// --------------------------- highlight words --------------------

	if ( m_select_tip < (int)m_tips.size() )
	{
		Frect r;

		vecTipsEx::iterator itb = m_tips.begin() + m_start_tip;
		vecTipsEx::iterator ite = m_tips.end();
		for ( u32 i = 0; itb != ite; ++itb, ++i ) // tips
		{
			TipString const& ts = (*itb);
			if ( (ts.HL_start < 0) || (ts.HL_finish < 0) || (ts.HL_start > ts.HL_finish) )
			{
				continue;
			}
			int    str_size = (int)ts.text.size();
			if ( (ts.HL_start >= str_size) || (ts.HL_finish > str_size) )
			{
				continue;
			}

			r.null();
			LPSTR  tmp      = (PSTR)_alloca( (str_size + 1) * sizeof(char) );

			strncpy_s( tmp, str_size+1, ts.text.c_str(), ts.HL_start );
			r.x1 = pr.x1 + w1 + pFont->SizeOf_( tmp );
			r.y1 = pr.y1 + i * font_h;

			strncpy_s( tmp, str_size+1, ts.text.c_str(), ts.HL_finish );
			r.x2 = pr.x1 + w1 + pFont->SizeOf_( tmp );
			r.y2 = r.y1 + font_h;

			DrawRect( r, tips_word_color );

			if ( i >= VIEW_TIPS_COUNT-1 )
			{
				break; // for itb
			}
		}// for itb
	} // if

	// --------------------------- scroll bar --------------------

	u32 tips_sz = m_tips.size();
	if ( tips_sz > VIEW_TIPS_COUNT )
	{
		Frect rb, rs;
		
		rb.x1 = pr.x2;
		rb.y1 = pr.y1;
		rb.x2 = rb.x1 + 2 * w1;
		rb.y2 = pr.y2;
		DrawRect( rb, tips_scroll_back_color );

		VERIFY( rb.y2 - rb.y1 >= 1.0f );
		float back_height = rb.y2 - rb.y1;
		float u_height = (back_height * VIEW_TIPS_COUNT)/ float(tips_sz);
		if ( u_height < 0.5f * font_h )
		{
			u_height = 0.5f * font_h;
		}

		//float u_pos = (back_height - u_height) * float(m_start_tip) / float(tips_sz);
		float u_pos = back_height * float(m_start_tip) / float(tips_sz);
		
		//clamp( u_pos, 0.0f, back_height - u_height );
		
		rs = rb;
		rs.y1 = pr.y1 + u_pos;
		rs.y2 = rs.y1 + u_height;
		DrawRect( rs, tips_scroll_pos_color );
	}

	UIRender->FlushPrimitive();
}

void CD3DConsole::DrawRect( Frect const& r, u32 color )
{
	UIRender->PushPoint( r.x1, r.y1, 0.0f, color, 0.0f, 0.0f );
	UIRender->PushPoint( r.x2, r.y1, 0.0f, color, 1.0f, 0.0f );
	UIRender->PushPoint( r.x2, r.y2, 0.0f, color, 1.0f, 1.0f );

	UIRender->PushPoint( r.x1, r.y1, 0.0f, color, 0.0f, 0.0f );
	UIRender->PushPoint( r.x2, r.y2, 0.0f, color, 1.0f, 1.0f );
	UIRender->PushPoint( r.x1, r.y2, 0.0f, color, 0.0f, 1.0f );
}
