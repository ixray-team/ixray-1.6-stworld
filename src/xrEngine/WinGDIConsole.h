#pragma once
#include "XR_IOConsole.h"


class CWinGDIConsole : public CConsole
{
private:
	typedef CConsole inherited;

private:
	HWND	m_hConsoleWnd;
	void	CreateConsoleWnd	( );
	
	HWND	m_hLogWnd;	
	void	CreateLogWnd		( );

	u32		m_dwStartLine;
	void	DrawLog				(HDC hDC, RECT* pRect);

private:
	HFONT	m_hLogWndFont;
	HFONT	m_hPrevFont;
	HBRUSH	m_hBackGroundBrush;

	HDC		m_hDC_LogWnd;
	HDC		m_hDC_LogWnd_BackBuffer;
	HBITMAP m_hBB_BM, m_hOld_BM;

	bool		m_bNeedUpdate;
	u32			m_dwLastUpdateTime;

public:
					CWinGDIConsole	( );
	virtual			~CWinGDIConsole	( );

	virtual	void		Initialize		( );
	virtual	void		Destroy			( );
	virtual void _BCL	OnFrame			( );

			void	OnPaint			( );
protected:

};// class CWinGDIConsole