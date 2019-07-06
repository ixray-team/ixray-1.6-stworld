#include "stdafx.h"
#include "UIOptionsItem.h"
#include "UIOptionsManagerScript.h"

void CUIOptionsManagerScript::SetCurrentValues(const char* group){
	CUIOptionsItem::GetOptionsManager()->SetCurrentValues(group);
}

void CUIOptionsManagerScript::SaveBackupValues(const char* group){
	CUIOptionsItem::GetOptionsManager()->SaveBackupValues(group);
}

void CUIOptionsManagerScript::SaveValues(const char* group){
	CUIOptionsItem::GetOptionsManager()->SaveValues(group);
}

void CUIOptionsManagerScript::UndoGroup(const char* group){
	CUIOptionsItem::GetOptionsManager()->UndoGroup(group);
}

void CUIOptionsManagerScript::OptionsPostAccept(){
	CUIOptionsItem::GetOptionsManager()->OptionsPostAccept();
}

void CUIOptionsManagerScript::SendMessage2Group(const char* group, const char* message){
	CUIOptionsItem::GetOptionsManager()->SendMessage2Group(group, message);
}

bool CUIOptionsManagerScript::NeedSystemRestart()
{
	return CUIOptionsItem::GetOptionsManager()->NeedSystemRestart();
}
bool CUIOptionsManagerScript::NeedVidRestart()
{
	return CUIOptionsItem::GetOptionsManager()->NeedVidRestart();
}

