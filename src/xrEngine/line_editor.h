////////////////////////////////////////////////////////////////////////////
//	Module 		: line_editor.h
//	Created 	: 22.02.2008
//	Author		: Evgeniy Sokolov
//	Description : line editor class, controller of line_edit_control
////////////////////////////////////////////////////////////////////////////

#ifndef LINE_EDITOR_H_INCLUDED
#define LINE_EDITOR_H_INCLUDED

#include "IInputReceiver.h"
#include "line_edit_control.h"

namespace text_editor
{

class line_editor_base 
{
public:
							line_editor_base	( u32 str_buffer_size );
	virtual					~line_editor_base	( );
	IC line_edit_control&	control				( )				{ return m_control; }
	virtual void			on_frame			( );

	virtual void			CaptureInput		( ) =0;
	virtual void			ReleaseInput		( ) =0;

protected:
	line_edit_control		m_control;
};

class line_editor_dinput :	public line_editor_base,
							public IInputReceiver
{
	typedef line_editor_base inherited;
public:
							line_editor_dinput	( u32 str_buffer_size );
	virtual					~line_editor_dinput	( );

	virtual void			on_frame			( );
	virtual void			CaptureInput		( );
	virtual void			ReleaseInput		( );

protected:
			void			update_key_states	( );

	virtual void			IR_OnKeyboardPress	( int dik );
	virtual void			IR_OnKeyboardHold	( int dik );
	virtual void			IR_OnKeyboardRelease( int dik );
}; // class line_editor_dinput


class line_editor_console :	public line_editor_base
{
	typedef line_editor_base inherited;
public:
							line_editor_console	( u32 str_buffer_size );
	virtual					~line_editor_console	( );

	virtual void			on_frame			( );
	virtual void			CaptureInput 		( );
	virtual void			ReleaseInput 		( );

			void			set_input_handle	( HANDLE h ) {m_console_input = h;}
protected:
	HANDLE					m_console_input;
			void			update_input		( );
};

} // namespace text_editor

#endif // LINE_EDITOR_H_INCLUDED
