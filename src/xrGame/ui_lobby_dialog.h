////////////////////////////////////////////////////////////////////////////
//	Created		: 10.01.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef UI_LOBBY_DIALOG_H_INCLUDED
#define UI_LOBBY_DIALOG_H_INCLUDED

#include "ui/uiDialogWnd.h"

class CUI3tButton;
class CUIEditBox;
class CUIStatic;
class lobby_menu;

class ui_lobby_dialog : public CUIDialogWnd
{
public:
						ui_lobby_dialog		( lobby_menu* parent );
	virtual				~ui_lobby_dialog	( );

public:
	lobby_menu*			m_parent;
	CUI3tButton*		m_fight_button;
	CUI3tButton*		m_login_button;
	CUIEditBox*			m_login_edit;
	CUIEditBox*			m_password_edit;
	CUIStatic*			m_status_static;
}; // class ui_lobby_dialog

#endif // #ifndef UI_LOBBY_DIALOG_H_INCLUDED