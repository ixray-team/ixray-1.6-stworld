////////////////////////////////////////////////////////////////////////////
//	Module 		: line_editor.cpp
//	Created 	: 22.02.2008
//	Author		: Evgeniy Sokolov
//	Description : line editor class implementation
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "line_editor.h"
#include "xr_input.h"

namespace text_editor
{

line_editor_base::line_editor_base( u32 str_buffer_size )
:m_control( str_buffer_size )
{}

line_editor_base::~line_editor_base()
{}

void line_editor_base::on_frame()
{
	// process input, then tick
	m_control.on_frame	( );
}

line_editor_dinput::line_editor_dinput( u32 str_buffer_size )
:inherited( str_buffer_size )
{
}

line_editor_dinput::~line_editor_dinput()
{
}

void line_editor_dinput::on_frame( )
{
	update_key_states	( );
	inherited::on_frame	( );
}

void line_editor_dinput::update_key_states( )
{
	m_control.set_key_state	( ks_LShift,	!!pInput->iGetAsyncKeyState(DIK_LSHIFT)		);
	m_control.set_key_state	( ks_RShift,	!!pInput->iGetAsyncKeyState(DIK_RSHIFT)		);
	m_control.set_key_state	( ks_LCtrl,		!!pInput->iGetAsyncKeyState(DIK_LCONTROL)	);
	m_control.set_key_state	( ks_RCtrl,		!!pInput->iGetAsyncKeyState(DIK_RCONTROL)	);
	m_control.set_key_state	( ks_LAlt,		!!pInput->iGetAsyncKeyState(DIK_LALT)		);
	m_control.set_key_state	( ks_RAlt,		!!pInput->iGetAsyncKeyState(DIK_RALT)		);
//	m_control.set_key_state	( ks_CapsLock,	text_editor::get_caps_lock_state()			);
}

void line_editor_dinput::CaptureInput( )
{
	IR_Capture();
}

void line_editor_dinput::ReleaseInput( )
{
	IR_Release();
}

void line_editor_dinput::IR_OnKeyboardPress( int dik )
{
	m_control.on_key_press( dik );
}

void line_editor_dinput::IR_OnKeyboardHold( int dik )
{
	m_control.on_key_hold( dik );
}

void line_editor_dinput::IR_OnKeyboardRelease( int dik )
{
	m_control.on_key_release( dik );
}


line_editor_console::line_editor_console( u32 str_buffer_size )
:inherited		(str_buffer_size),
m_console_input	( NULL )
{
	m_console_input		= GetStdHandle(STD_INPUT_HANDLE);
}

line_editor_console::~line_editor_console( )
{}

void line_editor_console::CaptureInput( )
{}

void line_editor_console::ReleaseInput( )
{}

void line_editor_console::on_frame()
{
	// update input, then tick
	if(m_console_input)
		update_input();

	inherited::on_frame();
}

void line_editor_console::update_input()
{
	DWORD num_events, num_events_read;
	BOOL res = GetNumberOfConsoleInputEvents( m_console_input, &num_events);
	
	if(num_events==0)
		return;

	INPUT_RECORD* buff = (INPUT_RECORD*)_alloca(num_events*sizeof(INPUT_RECORD));

	res = ReadConsoleInput( m_console_input, buff, num_events, &num_events_read );
	R_ASSERT(num_events==num_events_read);

	for(u32 i=0; i<num_events_read; ++i)
	{
		INPUT_RECORD* curr = buff+i;
		if(curr->EventType!=KEY_EVENT)
			continue;

		KEY_EVENT_RECORD& key_event = curr->Event.KeyEvent;

		// set key states
		DWORD key_states = key_event.dwControlKeyState;

		m_control.set_key_state	( ks_LShift,	key_states & SHIFT_PRESSED		);
		m_control.set_key_state	( ks_RShift,	key_states & SHIFT_PRESSED		);
		m_control.set_key_state	( ks_LCtrl,		key_states & LEFT_CTRL_PRESSED	);
		m_control.set_key_state	( ks_RCtrl,		key_states & RIGHT_CTRL_PRESSED	);
		m_control.set_key_state	( ks_LAlt,		key_states & LEFT_ALT_PRESSED	);
		m_control.set_key_state	( ks_RAlt,		key_states & RIGHT_ALT_PRESSED	);
		m_control.set_key_state	( ks_CapsLock,	key_states & CAPSLOCK_ON		);

		if(key_event.bKeyDown)
			m_control.on_key_press( key_event.wVirtualScanCode );
		else
			m_control.on_key_release( key_event.wVirtualScanCode );

	}
}

} // namespace text_editor
