#ifndef ENGINE_XRENGINE_IGAME_OBJECTPOOL_H_INCLUDED
#define ENGINE_XRENGINE_IGAME_OBJECTPOOL_H_INCLUDED

class ENGINE_API CObject;

class ENGINE_API Game_ObjectPool
{
public:

	static CObject*		create		( LPCSTR name );
	static void			destroy		( CObject* O );
};

#endif // #ifndef ENGINE_XRENGINE_IGAME_OBJECTPOOL_H_INCLUDED
