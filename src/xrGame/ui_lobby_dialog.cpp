////////////////////////////////////////////////////////////////////////////
//	Created		: 10.01.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ui_lobby_dialog.h"
#include "lobby_menu.h"
#include "ui/UIXmlInit.h"
#include "ui/UI3tButton.h"
#include "ui/UIEditBox.h"

ui_lobby_dialog::ui_lobby_dialog( lobby_menu* parent )
:m_parent(parent)
{
	CUIXmlInit				xml_init;

	CUIXml					uiXml;
	uiXml.Load				(CONFIG_PATH, UI_PATH,"lobby_dialog.xml");

	xml_init.InitWindow		( uiXml, "main", 0, this);

	m_fight_button			= xr_new<CUI3tButton>();
	m_fight_button->SetAutoDelete( true );
	AttachChild				( m_fight_button );
	xml_init.Init3tButton	( uiXml, "fight_button", 0, m_fight_button );

	m_login_button			= xr_new<CUI3tButton>();
	m_login_button->SetAutoDelete( true );
	AttachChild				( m_login_button );
	xml_init.Init3tButton	( uiXml, "login_button", 0, m_login_button );

	m_login_edit			= xr_new<CUIEditBox>();
	m_login_edit->SetAutoDelete( true );
	AttachChild				( m_login_edit );
	xml_init.InitEditBox	( uiXml, "login_edit", 0, m_login_edit );

	m_password_edit			= xr_new<CUIEditBox>();
	m_password_edit->SetAutoDelete( true );
	AttachChild				( m_password_edit );
	xml_init.InitEditBox	( uiXml, "password_edit", 0, m_password_edit );

	m_status_static			= xr_new<CUIStatic>();
	m_status_static->SetAutoDelete( true );
	AttachChild				( m_status_static );
	xml_init.InitStatic		( uiXml, "status_static", 0, m_status_static );

	m_parent->Register		( m_fight_button );
	m_parent->Register		( m_login_button );

	m_parent->AddCallback( m_fight_button,BUTTON_CLICKED,   CUIWndCallback::void_function(m_parent, &lobby_menu::on_btn_figth_clicked));
	m_parent->AddCallback( m_login_button,BUTTON_CLICKED,   CUIWndCallback::void_function(m_parent, &lobby_menu::on_btn_login_clicked));

}

ui_lobby_dialog::~ui_lobby_dialog( )
{
}
