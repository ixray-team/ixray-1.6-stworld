////////////////////////////////////////////////////////////////////////////
//	Created		: 01.02.2012
//	Author		: Andrew Kolomiets
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_XRGAME_XRSERVERINFO_H_INCLUDED
#define ENGINE_XRGAME_XRSERVERINFO_H_INCLUDED

class ENGINE_API CServerInfo
{
private:
	struct SItem_ServerInfo
	{
		string128	name;
		u32			color;
	};
	enum { max_item = 15 };
	svector<SItem_ServerInfo,max_item>	data;

public:
	u32		Size()			{ return data.size(); }
	void	ResetData()		{ data.clear(); }

	void	AddItem( LPCSTR name_,		LPCSTR value_, u32 color_ = RGB(255,255,255) );
	void	AddItem( shared_str& name_,	LPCSTR value_, u32 color_ = RGB(255,255,255) );

	IC SItem_ServerInfo&	operator[] ( u32 id ) { VERIFY( id < max_item ); return data[id]; }

	CServerInfo() {};
	~CServerInfo() {};
};

#endif // #ifndef ENGINE_XRGAME_XRSERVERINFO_H_INCLUDED