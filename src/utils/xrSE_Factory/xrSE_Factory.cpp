////////////////////////////////////////////////////////////////////////////
//	Module 		: xrSE_Factory.cpp
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creatore
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "xrSE_Factory.h"
#include "object_factory.h"
#include "xrEProps.h"
#include "xrSE_Factory_import_export.h"

#pragma comment(lib,"xrCore.lib")

extern CSE_Abstract *F_entity_Create	(LPCSTR section);

extern HMODULE						prop_helper_module;

struct CChooseType {};

typedef IPropHelper& (__stdcall *TPHelper) ();

TPHelper					_PHelper = 0;
HMODULE						prop_helper_module = 0;
LPCSTR						prop_helper_library = "xrEPropsB.dll", prop_helper_func = "PHelper";


void load_prop_helper( )
{
	prop_helper_module		= LoadLibrary(prop_helper_library);
	if (!prop_helper_module) {
		Msg					("! Cannot find library %s",prop_helper_library);
		return;
	}
	_PHelper				= (TPHelper)GetProcAddress(prop_helper_module,prop_helper_func);
	if (!_PHelper) {
		Msg					("! Cannot find entry point of the function %s in the library %s",prop_helper_func,prop_helper_func);
		return;
	}
}

IPropHelper &PHelper()
{
	static	bool			first_time = true;
	if (first_time) {
		first_time			= false;
		load_prop_helper	();
	}
	R_ASSERT3				(_PHelper, "Cannot find entry point of the function or Cannot find library",prop_helper_library);
	return					(_PHelper());
}

#ifdef NDEBUG

namespace std {
	void terminate			()
	{
		abort				();
	}
} // namespace std

#endif // #ifdef NDEBUG

extern "C" {
	FACTORY_API	ISE_Abstract* __stdcall create_entity	(LPCSTR section)
	{
		return					(F_entity_Create(section));
	}

	FACTORY_API	void		__stdcall destroy_entity	(ISE_Abstract *&abstract)
	{
		CSE_Abstract			*object = smart_cast<CSE_Abstract*>(abstract);
		F_entity_Destroy		(object);
		abstract				= 0;
	}
};

//void setup_luabind_allocator		();

//#define TRIVIAL_ENCRYPTOR_DECODER
//#include UP(xrEngine/trivial_encryptor.h)

BOOL APIENTRY DllMain		(HANDLE module_handle, DWORD call_reason, LPVOID reserved)
{
	switch (call_reason) {
		case DLL_PROCESS_ATTACH: {
//			g_temporary_stuff			= &trivial_encryptor::decode;

			Debug._initialize			(false);
 			Core._initialize			("xrSE_Factory",NULL,TRUE,"fsfactory.ltx");
			string_path					SYSTEM_LTX;
			FS.update_path				(SYSTEM_LTX,"$game_config$","system.ltx");
			pSettings					= xr_new<CInifile>(SYSTEM_LTX);

//			setup_luabind_allocator		();

			break;
		}
		case DLL_PROCESS_DETACH: {

			xr_delete					(g_object_factory);
			CInifile** s				= (CInifile**)(&pSettings);
			xr_delete					(*s);
			xr_delete					(g_object_factory);
			if (prop_helper_module)
				FreeLibrary				(prop_helper_module);
			Core._destroy				();
			break;
		}
	}
    return				(TRUE);
}

//void _destroy_item_data_vector_cont(T_VECTOR* vec)
//{
//	T_VECTOR::iterator it		= vec->begin();
//	T_VECTOR::iterator it_e		= vec->end();
//
//	xr_vector<CUIXml*>			_tmp;	
//	for(;it!=it_e;++it)
//	{
//		xr_vector<CUIXml*>::iterator it_f = std::find(_tmp.begin(), _tmp.end(), (*it)._xml);
//		if(it_f==_tmp.end())
//			_tmp.push_back	((*it)._xml);
//	}
//	delete_data	(_tmp);
//}
