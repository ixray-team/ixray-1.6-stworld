#ifndef UIGAMEMP_H
#define UIGAMEMP_H

#include "UIGameCustom.h"

class CUIAchivementIndicator;
class game_cl_mp;

class UIGameMP : public CUIGameCustom
{	
	typedef CUIGameCustom inherited;
public:
					UIGameMP			();
	virtual			~UIGameMP			();
	
	void			AddAchivment			(shared_str const & achivement_name,
											 shared_str const & color_animation,
											 u32 const width,
											 u32 const height);

	virtual bool 	IR_UIOnKeyboardPress	(int dik);
	virtual bool 	IR_UIOnKeyboardRelease	(int dik);
	virtual void	SetClGame				(game_cl_GameState* g);

protected:
	CUIAchivementIndicator*		m_pAchivementIdicator;
	game_cl_mp*					m_game;
}; //class UIGameMP

#endif //#ifndef UIGAMEMP_H