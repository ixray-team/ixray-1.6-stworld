//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
//	Oles		- Oles Shishkovtsov
//	AlexMX		- Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "../xrNetServer/NET_AuthCheck.h"

#include "xr_input.h"
#include "xr_ioc_cmd.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "../xrcdb/ispatial.h"

#ifndef DEDICATED_SERVER
	#include "D3DConsole.h"
#else
	#include "WinGDIConsole.h"
	#include "TextConsole.h"
#endif //#ifndef DEDICATED_SERVER

#include <process.h>
#include <locale.h>

#include <luabind/config.hpp>
#include <luabind/luabind_memory.h>

#pragma comment( lib, "xrCore.lib"	)
#pragma comment( lib, "xrCDB.lib"	)
#pragma comment( lib, "xrSound.lib"	)

#pragma comment( lib, "xrAPI.lib"	)

#pragma comment( lib, "winmm.lib"		)

#pragma comment( lib, "d3d9.lib"		)
#pragma comment( lib, "dinput8.lib"		)
#pragma comment( lib, "dxguid.lib"		)

//---------------------------------------------------------------------
ENGINE_API CInifile* pGameIni		= NULL;

// computing build id
XRCORE_API	LPCSTR	build_date;
XRCORE_API	u32		build_id;

#ifdef DEDICATED_SERVER
	bool g_pure_console = false;
#endif //#ifdef DEDICATED_SERVER

#ifdef MASTER_GOLD
//#	define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD


static LPSTR month_id[12] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

static int days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int start_day	= 31;	// 31
static int start_month	= 1;	// January
static int start_year	= 1999;	// 1999

//#ifndef DEDICATED_SERVER
//#include "../xrGameSpy/gamespy/md5c.c"
//#include <ctype.h>
//#define DEFAULT_MODULE_HASH "3CAABCFCFF6F3A810019C6A72180F166"
//static char szEngineHash[33] = DEFAULT_MODULE_HASH;
//
//char* ComputeModuleHash( char* pszHash )
//{
//	char szModuleFileName[ MAX_PATH ];
//	HANDLE hModuleHandle = NULL , hFileMapping = NULL;
//	LPVOID lpvMapping = NULL;
//	MEMORY_BASIC_INFORMATION MemoryBasicInformation;
//
//	if ( ! GetModuleFileName( NULL , szModuleFileName , MAX_PATH ) )
//		return pszHash;
//
//	hModuleHandle = CreateFile( szModuleFileName , GENERIC_READ , FILE_SHARE_READ , NULL , OPEN_EXISTING , 0 , NULL );
//
//	if ( hModuleHandle == INVALID_HANDLE_VALUE )
//		return pszHash;
//
//	hFileMapping = CreateFileMapping( hModuleHandle , NULL , PAGE_READONLY , 0 , 0 , NULL );
//
//	if ( hFileMapping == NULL ) {
//		CloseHandle( hModuleHandle );
//		return pszHash;
//	}
//
//	lpvMapping = MapViewOfFile( hFileMapping , FILE_MAP_READ , 0 , 0 , 0 );
//
//	if ( lpvMapping == NULL ) {
//		CloseHandle( hFileMapping );
//		CloseHandle( hModuleHandle );
//		return pszHash;
//	}
//
//	ZeroMemory( &MemoryBasicInformation , sizeof( MEMORY_BASIC_INFORMATION ) );
//
//	VirtualQuery( lpvMapping , &MemoryBasicInformation , sizeof( MEMORY_BASIC_INFORMATION ) );
//
//	if ( MemoryBasicInformation.RegionSize ) {
//		char szHash[33];
//		MD5Digest( ( unsigned char *)lpvMapping , (unsigned int) MemoryBasicInformation.RegionSize , szHash );
//		MD5Digest( ( unsigned char *)szHash , 32 , pszHash );
//		for ( int i = 0 ; i < 32 ; ++i )
//			pszHash[ i ] = (char)toupper( pszHash[ i ] );
//	}
//
//	UnmapViewOfFile( lpvMapping );
//	CloseHandle( hFileMapping );
//	CloseHandle( hModuleHandle );
//
//	return pszHash;
//}
//#endif // DEDICATED_SERVER

void compute_build_id( )
{
	build_date			= __DATE__;

	int					days;
	int					months = 0;
	int					years;
	string16			month;
	string256			buffer;
	xr_strcpy				(buffer,__DATE__);
	sscanf				(buffer,"%s %d %d",month,&days,&years);

	for (int i=0; i<12; i++) {
		if (_stricmp(month_id[i],month))
			continue;

		months			= i;
		break;
	}

	build_id			= (years - start_year)*365 + days - start_day;

	for (int i=0; i<months; ++i)
		build_id		+= days_in_month[i];

	for (int i=0; i<start_month-1; ++i)
		build_id		-= days_in_month[i];
}
//---------------------------------------------------------------------
// 2446363
// umbt@ukr.net
//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor	: public pureFrame
{
	virtual void	_BCL	OnFrame	( )
	{
		//Msg							("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
		Device.Statistic->Sound.Begin();
		::Sound->update				(Device.vCameraPosition,Device.vCameraDirection,Device.vCameraTop);
		Device.Statistic->Sound.End	();
	}
}	SoundProcessor;

//////////////////////////////////////////////////////////////////////////
// global variables
ENGINE_API	CApplication*	pApp			= NULL;
static		HWND			logoWindow		= NULL;

//			int				doLauncher		();
			void			doBenchmark		(LPCSTR name);
ENGINE_API	bool			g_bBenchmark	= false;
string512	g_sBenchmarkName;


ENGINE_API	string512		g_sLaunchOnExit_params;
ENGINE_API	string512		g_sLaunchOnExit_app;
ENGINE_API	string_path		g_sLaunchWorkingFolder;
// -------------------------------------------
// startup point
void InitEngine		()
{
	Engine.Initialize			( );
	Device.Initialize			( );
}

struct path_excluder_predicate
{
	explicit path_excluder_predicate(xr_auth_strings_t const * ignore) :
		m_ignore(ignore)
	{
	}
	bool xr_stdcall is_allow_include(LPCSTR path)
	{
		if (!m_ignore)
			return true;
		
		return allow_to_include_path(*m_ignore, path);
	}
	xr_auth_strings_t const *	m_ignore;
};

void InitSettings	()
{
	//#ifndef DEDICATED_SERVER
	//	Msg( "EH: %s\n" , ComputeModuleHash( szEngineHash ) );
	//#endif // DEDICATED_SERVER

	string_path					fname; 
	FS.update_path				(fname,"$game_config$","system.ltx");
#ifdef DEBUG
	Msg							("Updated path to system.ltx is %s", fname);
#endif // #ifdef DEBUG
	pSettings					= xr_new<CInifile>	(fname,TRUE);
	CHECK_OR_EXIT				(0!=pSettings->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.",fname));

	xr_auth_strings_t			tmp_ignore_pathes;
	xr_auth_strings_t			tmp_check_pathes;
	//fill_auth_check_params		(tmp_ignore_pathes, tmp_check_pathes);
	
	path_excluder_predicate			tmp_excluder(&tmp_ignore_pathes);
	CInifile::allow_include_func_t	tmp_functor;
	tmp_functor.bind(&tmp_excluder, &path_excluder_predicate::is_allow_include);

	FS.update_path				(fname,"$game_config$","game.ltx");
	pGameIni					= xr_new<CInifile>	(fname,TRUE);
	CHECK_OR_EXIT				(0!=pGameIni->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.",fname));
}

void InitConsole	()
{
		pConsoleCommands				= xr_new<command_storage>	();
#ifdef DEDICATED_SERVER
	{
		if (g_pure_console)
			pConsole						= xr_new<CTextConsole>	();		
		else
			pConsole						= xr_new<CWinGDIConsole>	();		
	}
#else
	//	else
	{
		pConsole						= xr_new<CD3DConsole>	();
	}
#endif
	pConsole->Initialize			( );

	xr_strcpy						(pConsoleCommands->m_ConfigFile, "user.ltx");
	if (strstr(Core.Params,"-ltx ")) {
		string64				c_name;
		sscanf					(strstr(Core.Params,"-ltx ")+5,"%[^ ] ",c_name);
		xr_strcpy				(pConsoleCommands->m_ConfigFile,c_name);
	}
}

void InitInput		()
{
	BOOL bCaptureInput			= 
#ifndef DEDICATED_SERVER
		!strstr(Core.Params,"-i");
#else
	FALSE;
#endif //#ifndef DEDICATED_SERVER

	pInput						= xr_new<CInput>		(bCaptureInput);
}
void destroyInput	()
{
	xr_delete					( pInput		);
}

void InitSound1		()
{
	bool nosound = 
#ifndef DEDICATED_SERVER
		NULL != strstr( Core.Params,"-nosound");
#else
		true;
#endif //#ifndef DEDICATED_SERVER

	CSound_manager_interface::_create(0, nosound);
}

void InitSound2		()
{
	CSound_manager_interface::_create(1, false);
}

void destroySound	()
{
	CSound_manager_interface::_destroy				( );
}

void destroySettings()
{
	CInifile** s				= (CInifile**)(&pSettings);
	xr_delete					( *s		);
	xr_delete					( pGameIni		);
}

void destroyConsole	()
{
	pConsoleCommands->Execute	( "cfg_save" );
	pConsole->Destroy			( );
	xr_delete					( pConsole );
	pConsoleCommands->Destroy	( );
	xr_delete					( pConsoleCommands );
}

void destroyEngine	()
{
	Device.Destroy				( );
	Engine.Destroy				( );
}

void execUserScript				( )
{
	pConsoleCommands->Execute		("default_controls");
	pConsoleCommands->ExecuteScript	(pConsoleCommands->m_ConfigFile);
}

static LPVOID CS_CALL luabind_allocator	(
		luabind::memory_allocation_function_parameter const,
		void const * const pointer,
		size_t const size
	)
{
	if (!size) {
		LPVOID	non_const_pointer = const_cast<LPVOID>(pointer);
		xr_free	(non_const_pointer);
		return	( 0 );
	}

	if (!pointer) {
#ifdef DEBUG
		return	( Memory.mem_alloc(size, "luabind") );
#else // #ifdef DEBUG
		return	( Memory.mem_alloc(size) );
#endif // #ifdef DEBUG
	}

	LPVOID		non_const_pointer = const_cast<LPVOID>(pointer);
#ifdef DEBUG
	return		( Memory.mem_realloc(non_const_pointer, size, "luabind") );
#else // #ifdef DEBUG
	return		( Memory.mem_realloc(non_const_pointer, size) );
#endif // #ifdef DEBUG
}

void setup_luabind_allocator		()
{
	luabind::allocator				= &luabind_allocator;
	luabind::allocator_parameter	= 0;
}

void Startup()
{
	InitSound1		();
	execUserScript	();
	InitSound2		();

	// ...command line for auto start
	{
		LPCSTR	pStartup			= strstr				(Core.Params,"-start ");
		if (pStartup)				
			pConsoleCommands->Execute		(pStartup+1);
	}

	setup_luabind_allocator		( );
	// Initialize APP
//#ifndef DEDICATED_SERVER
	ShowWindow					( Device.m_hWnd , SW_SHOWNORMAL );
	Device.Create				( );
//#endif
	LALib.OnCreate				( );
	pApp						= xr_new<CApplication>	();
	g_pGamePersistent			= (IGame_Persistent*)	NEW_INSTANCE (CLSID_GAME_PERSISTANT);
	g_SpatialSpace				= xr_new<ISpatial_DB>	();
	g_SpatialSpacePhysic		= xr_new<ISpatial_DB>	();
	
	// Destroy LOGO
	DestroyWindow				(logoWindow);
	logoWindow					= NULL;

	// Main cycle
	Memory.mem_usage();
	Device.Run					( );

	// Destroy APP
	xr_delete					( g_SpatialSpacePhysic	);
	xr_delete					( g_SpatialSpace		);
	DEL_INSTANCE				( g_pGamePersistent		);
	xr_delete					( pApp					);
	Engine.Event.Dump			( );

	// Destroying
	destroyInput();

	destroySettings();

	LALib.OnDestroy				( );
	
	destroyConsole();

	destroySound();

	destroyEngine();
}

static BOOL CALLBACK logDlgProc( HWND hw, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ){
		case WM_DESTROY:
			break;
		case WM_CLOSE:
			DestroyWindow( hw );
			break;
		case WM_COMMAND:
			if( LOWORD(wp)==IDCANCEL )
				DestroyWindow( hw );
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

extern void	testbed	(void);

#define dwStickyKeysStructSize sizeof( STICKYKEYS )
#define dwFilterKeysStructSize sizeof( FILTERKEYS )
#define dwToggleKeysStructSize sizeof( TOGGLEKEYS )

struct damn_keys_filter {
	BOOL bScreenSaverState;

	// Sticky & Filter & Toggle keys

	STICKYKEYS StickyKeysStruct;
	FILTERKEYS FilterKeysStruct;
	TOGGLEKEYS ToggleKeysStruct;

	DWORD dwStickyKeysFlags;
	DWORD dwFilterKeysFlags;
	DWORD dwToggleKeysFlags;

	damn_keys_filter	()
	{
		// Screen saver stuff

		bScreenSaverState = FALSE;

		// Saveing current state
		SystemParametersInfo( SPI_GETSCREENSAVEACTIVE , 0 , ( PVOID ) &bScreenSaverState , 0 );

		if ( bScreenSaverState )
			// Disable screensaver
			SystemParametersInfo( SPI_SETSCREENSAVEACTIVE , FALSE , NULL , 0 );

		dwStickyKeysFlags = 0;
		dwFilterKeysFlags = 0;
		dwToggleKeysFlags = 0;


		ZeroMemory( &StickyKeysStruct , dwStickyKeysStructSize );
		ZeroMemory( &FilterKeysStruct , dwFilterKeysStructSize );
		ZeroMemory( &ToggleKeysStruct , dwToggleKeysStructSize );

		StickyKeysStruct.cbSize = dwStickyKeysStructSize;
		FilterKeysStruct.cbSize = dwFilterKeysStructSize;
		ToggleKeysStruct.cbSize = dwToggleKeysStructSize;

		// Saving current state
		SystemParametersInfo( SPI_GETSTICKYKEYS , dwStickyKeysStructSize , ( PVOID ) &StickyKeysStruct , 0 );
		SystemParametersInfo( SPI_GETFILTERKEYS , dwFilterKeysStructSize , ( PVOID ) &FilterKeysStruct , 0 );
		SystemParametersInfo( SPI_GETTOGGLEKEYS , dwToggleKeysStructSize , ( PVOID ) &ToggleKeysStruct , 0 );

		if ( StickyKeysStruct.dwFlags & SKF_AVAILABLE ) {
			// Disable StickyKeys feature
			dwStickyKeysFlags = StickyKeysStruct.dwFlags;
			StickyKeysStruct.dwFlags = 0;
			SystemParametersInfo( SPI_SETSTICKYKEYS , dwStickyKeysStructSize , ( PVOID ) &StickyKeysStruct , 0 );
		}

		if ( FilterKeysStruct.dwFlags & FKF_AVAILABLE ) {
			// Disable FilterKeys feature
			dwFilterKeysFlags = FilterKeysStruct.dwFlags;
			FilterKeysStruct.dwFlags = 0;
			SystemParametersInfo( SPI_SETFILTERKEYS , dwFilterKeysStructSize , ( PVOID ) &FilterKeysStruct , 0 );
		}

		if ( ToggleKeysStruct.dwFlags & TKF_AVAILABLE ) {
			// Disable FilterKeys feature
			dwToggleKeysFlags = ToggleKeysStruct.dwFlags;
			ToggleKeysStruct.dwFlags = 0;
			SystemParametersInfo( SPI_SETTOGGLEKEYS , dwToggleKeysStructSize , ( PVOID ) &ToggleKeysStruct , 0 );
		}
	}

	~damn_keys_filter	()
	{
		if ( bScreenSaverState )
			// Restoring screen saver
			SystemParametersInfo( SPI_SETSCREENSAVEACTIVE , TRUE , NULL , 0 );

		if ( dwStickyKeysFlags) {
			// Restore StickyKeys feature
			StickyKeysStruct.dwFlags = dwStickyKeysFlags;
			SystemParametersInfo( SPI_SETSTICKYKEYS , dwStickyKeysStructSize , ( PVOID ) &StickyKeysStruct , 0 );
		}

		if ( dwFilterKeysFlags ) {
			// Restore FilterKeys feature
			FilterKeysStruct.dwFlags = dwFilterKeysFlags;
			SystemParametersInfo( SPI_SETFILTERKEYS , dwFilterKeysStructSize , ( PVOID ) &FilterKeysStruct , 0 );
		}

		if ( dwToggleKeysFlags ) {
			// Restore FilterKeys feature
			ToggleKeysStruct.dwFlags = dwToggleKeysFlags;
			SystemParametersInfo( SPI_SETTOGGLEKEYS , dwToggleKeysStructSize , ( PVOID ) &ToggleKeysStruct , 0 );
		}

	}
};

#undef dwStickyKeysStructSize
#undef dwFilterKeysStructSize
#undef dwToggleKeysStructSize

// Фунция для тупых требований THQ и тупых американских пользователей
BOOL IsOutOfVirtualMemory()
{
#define VIRT_ERROR_SIZE 256
#define VIRT_MESSAGE_SIZE 512

	MEMORYSTATUSEX statex;
	DWORD dwPageFileInMB = 0;
	DWORD dwPhysMemInMB = 0;
	HINSTANCE hApp = 0;
	char	pszError[ VIRT_ERROR_SIZE ];
	char	pszMessage[ VIRT_MESSAGE_SIZE ];

	ZeroMemory( &statex , sizeof( MEMORYSTATUSEX ) );
	statex.dwLength = sizeof( MEMORYSTATUSEX );
	
	if ( ! GlobalMemoryStatusEx( &statex ) )
		return 0;

	dwPageFileInMB = ( DWORD ) ( statex.ullTotalPageFile / ( 1024 * 1024 ) ) ;
	dwPhysMemInMB = ( DWORD ) ( statex.ullTotalPhys / ( 1024 * 1024 ) ) ;

	// Довольно отфонарное условие
	if ( ( dwPhysMemInMB > 500 ) && ( ( dwPageFileInMB + dwPhysMemInMB ) > 2500  ) )
		return 0;

	hApp = GetModuleHandle( NULL );

	if ( ! LoadString( hApp , RC_VIRT_MEM_ERROR , pszError , VIRT_ERROR_SIZE ) )
		return 0;
 
	if ( ! LoadString( hApp , RC_VIRT_MEM_TEXT , pszMessage , VIRT_MESSAGE_SIZE ) )
		return 0;

	MessageBox( NULL , pszMessage , pszError , MB_OK | MB_ICONHAND );

	return 1;	
}

#include "xr_ioc_cmd.h"


ENGINE_API	bool g_dedicated_server	= false;

#ifndef DEDICATED_SERVER
	// forward declaration for Parental Control checks
	BOOL IsPCAccessAllowed(); 
#endif // DEDICATED_SERVER

int APIENTRY WinMain_impl(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     char *    lpCmdLine,
                     int       nCmdShow)
{
#ifdef DEDICATED_SERVER
	Debug._initialize			(true);
#else // DEDICATED_SERVER
	Debug._initialize			(false);
#endif // DEDICATED_SERVER

	if (!IsDebuggerPresent()) 
	{

		HMODULE const kernel32	= LoadLibrary("kernel32.dll");
		R_ASSERT				(kernel32);

		typedef BOOL (__stdcall*HeapSetInformation_type) (HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
		HeapSetInformation_type const heap_set_information = 
			(HeapSetInformation_type)GetProcAddress(kernel32, "HeapSetInformation");
		if (heap_set_information) {
			ULONG HeapFragValue	= 2;
#ifdef DEBUG
			BOOL const result	= 
#endif // #ifdef DEBUG
				heap_set_information(
					GetProcessHeap(),
					HeapCompatibilityInformation,
					&HeapFragValue,
					sizeof(HeapFragValue)
				);
			VERIFY2				(result, "can't set process heap low fragmentation");
		}
	}


#ifndef DEDICATED_SERVER
	// Check for virtual memory
	if ( ( strstr( lpCmdLine , "--skipmemcheck" ) == NULL ) && IsOutOfVirtualMemory() )
		return 0;

	// Parental Control for Vista and upper
	if ( ! IsPCAccessAllowed() ) {
		MessageBox( NULL , "Access restricted" , "Parental Control" , MB_OK | MB_ICONERROR );
		return 1;
	}

	// Check for another instance
#ifdef NO_MULTI_INSTANCES
	#define STALKER_PRESENCE_MUTEX "Local\\STALKER-COP"
	
	HANDLE hCheckPresenceMutex = INVALID_HANDLE_VALUE;
	hCheckPresenceMutex = OpenMutex( READ_CONTROL , FALSE ,  STALKER_PRESENCE_MUTEX );
	if ( hCheckPresenceMutex == NULL ) {
		// New mutex
		hCheckPresenceMutex = CreateMutex( NULL , FALSE , STALKER_PRESENCE_MUTEX );
		if ( hCheckPresenceMutex == NULL )
			// Shit happens
			return 2;
	} else {
		// Already running
		CloseHandle( hCheckPresenceMutex );
		return 1;
	}
#endif
#else // DEDICATED_SERVER
	g_dedicated_server		= true;
#endif // DEDICATED_SERVER

	SetThreadAffinityMask		(GetCurrentThread(),1);

	// Title window
	logoWindow					= CreateDialog(GetModuleHandle(NULL),	MAKEINTRESOURCE(IDD_STARTUP), 0, logDlgProc );
	
	HWND logoPicture			= GetDlgItem(logoWindow, IDC_STATIC_LOGO);
	RECT logoRect;
	GetWindowRect(logoPicture, &logoRect);

	SetWindowPos				(
		logoWindow,
#ifndef DEBUG
		HWND_TOPMOST,
#else
		HWND_NOTOPMOST,
#endif // NDEBUG
		0,
		0,
		logoRect.right - logoRect.left,
		logoRect.bottom - logoRect.top,
		SWP_NOMOVE | SWP_SHOWWINDOW// | SWP_NOSIZE
	);
	UpdateWindow(logoWindow);

	g_sLaunchOnExit_app[0]		= NULL;
	g_sLaunchOnExit_params[0]	= NULL;

	LPCSTR						fsgame_ltx_name = "-fsltx ";
	string_path					fsgame = "";
	if (strstr(lpCmdLine, fsgame_ltx_name)) {
		int						sz = xr_strlen(fsgame_ltx_name);
		sscanf					(strstr(lpCmdLine,fsgame_ltx_name)+sz,"%[^ ] ",fsgame);
	}

	compute_build_id			();
	Core._initialize			("xray", NULL, TRUE, fsgame[0] ? fsgame : NULL);


	InitSettings				( );

	// Adjust player & computer name for Asian
	if ( pSettings->line_exist( "string_table" , "no_native_input" ) ) {
			xr_strcpy( Core.UserName , sizeof( Core.UserName ) , "Player" );
			xr_strcpy( Core.CompName , sizeof( Core.CompName ) , "Computer" );
	}

#ifdef DEDICATED_SERVER
		g_pure_console			= true;//(NULL!=strstr(Core.Params, "-pure_console"));
#endif //DEDICATED_SERVER

#ifndef DEDICATED_SERVER
	{
		damn_keys_filter		filter;
		(void)filter;
#endif // DEDICATED_SERVER

		FPU::m24r				();
		InitEngine				();

#ifdef DEDICATED_SERVER
	if(!g_pure_console)
		InitInput				(); // because WinGdiConsole still use dinput.
#else
		InitInput				();
#endif //DEDICATED_SERVER

		InitConsole				();

		Engine.External.CreateRendererList();

		LPCSTR benchName = "-batch_benchmark ";
		if(strstr(lpCmdLine, benchName))
		{
			int sz = xr_strlen(benchName);
			string64				b_name;
			sscanf					(strstr(Core.Params,benchName)+sz,"%[^ ] ",b_name);
			doBenchmark				(b_name);
			return 0;
		}

		Msg("command line %s", lpCmdLine);
#ifndef DEDICATED_SERVER
		if(strstr(Core.Params,"-r2a"))	
			pConsoleCommands->Execute			("renderer renderer_r2a");
		else
		if(strstr(Core.Params,"-r2"))	
			pConsoleCommands->Execute			("renderer renderer_r2");
		else
		{
			CCC_LoadCFG_custom*	pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
			pTmp->Execute				(pConsoleCommands->m_ConfigFile);
			xr_delete					(pTmp);
		}
#else
		pConsoleCommands->Execute("renderer renderer_r1");
#endif
//.		InitInput					( );
		Engine.External.Initialize	( );
		pConsoleCommands->Execute			("stat_memory");

		Startup	 					( );
		Core._destroy				( );

		// check for need to execute something external
		if (/*xr_strlen(g_sLaunchOnExit_params) && */xr_strlen(g_sLaunchOnExit_app) ) 
		{
			//CreateProcess need to return results to next two structures
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			//We use CreateProcess to setup working folder
			char const * temp_wf = (xr_strlen(g_sLaunchWorkingFolder) > 0) ? g_sLaunchWorkingFolder : NULL;
			CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, NULL, NULL, FALSE, 0, NULL, 
				temp_wf, &si, &pi);

		}
#ifndef DEDICATED_SERVER
#ifdef NO_MULTI_INSTANCES		
		// Delete application presence mutex
		CloseHandle( hCheckPresenceMutex );
#endif
	}
	// here damn_keys_filter class instanse will be destroyed
#endif // DEDICATED_SERVER

	return						0;
}

int stack_overflow_exception_filter	(int exception_code)
{
   if (exception_code == EXCEPTION_STACK_OVERFLOW)
   {
       // Do not call _resetstkoflw here, because
       // at this point, the stack is not yet unwound.
       // Instead, signal that the handler (the __except block)
       // is to be executed.
       return EXCEPTION_EXECUTE_HANDLER;
   }
   else
       return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     char *    lpCmdLine,
                     int       nCmdShow)
{
	__try 
	{
		WinMain_impl		(hInstance,hPrevInstance,lpCmdLine,nCmdShow);
	}
	__except(stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw		();
		FATAL				("stack overflow");
	}

	return					(0);
}

LPCSTR _GetFontTexName (LPCSTR section)
{
	static char* tex_names[]={"texture800","texture","texture1600"};
	int def_idx		= 1;//default 1024x768
	int idx			= def_idx;

#if 0
	u32 w = Device.dwWidth;

	if(w<=800)		idx = 0;
	else if(w<=1280)idx = 1;
	else 			idx = 2;
#else
	u32 h = Device.dwHeight;

	if(h<=600)		idx = 0;
	else if(h<1024)	idx = 1;
	else 			idx = 2;
#endif

	while(idx>=0){
		if( pSettings->line_exist(section,tex_names[idx]) )
			return pSettings->r_string(section,tex_names[idx]);
		--idx;
	}
	return pSettings->r_string(section,tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = _GetFontTexName(section);
	R_ASSERT(font_tex_name);

	LPCSTR sh_name = pSettings->r_string(section,"shader");
	if(!F){
		F = xr_new<CGameFont> (sh_name, font_tex_name, flags);
	}else
		F->Initialize(sh_name, font_tex_name);

	if (pSettings->line_exist(section,"size")){
		float sz = pSettings->r_float(section,"size");
		if (flags&CGameFont::fsDeviceIndependent)	F->SetHeightI(sz);
		else										F->SetHeight(sz);
	}
	if (pSettings->line_exist(section,"interval"))
		F->SetInterval(pSettings->r_fvector2(section,"interval"));

}

CApplication::CApplication()
{
	ll_dwReference	= 0;

	max_load_stage = 0;

	// events
	eQuit						= Engine.Event.Handler_Attach("KERNEL:quit",this);
	eStart						= Engine.Event.Handler_Attach("KERNEL:start",this);
	eStartLobby					= Engine.Event.Handler_Attach("KERNEL:lobby",this);
	eDisconnect					= Engine.Event.Handler_Attach("KERNEL:disconnect",this);

	// levels
	Level_Scan					( );

	// Font
	pFontSystem					= NULL;

	// Register us
	Device.seqFrame.Add			(this, REG_PRIORITY_HIGH+1000);
	
	if (psDeviceFlags.test(mtSound))	Device.seqFrameMT.Add		(&SoundProcessor);
	else								Device.seqFrame.Add			(&SoundProcessor);

	pConsole->Show				( );

	// App Title
	ls_header[ 0 ] = '\0';
	ls_tip_number[ 0 ] = '\0';
	ls_tip[ 0 ] = '\0';
}

CApplication::~CApplication()
{
	pConsole->Hide				( );

	// font
	xr_delete					( pFontSystem		);

	Device.seqFrameMT.Remove	(&SoundProcessor);
	Device.seqFrame.Remove		(&SoundProcessor);
	Device.seqFrame.Remove		(this);


	// events
	Engine.Event.Handler_Detach	(eDisconnect,this);
	Engine.Event.Handler_Detach	(eStart,this);
	Engine.Event.Handler_Detach	(eStartLobby,this);
	Engine.Event.Handler_Detach	(eQuit,this);
}

extern CRenderDevice Device;

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if(E==eQuit)
	{
		PostQuitMessage	(0);
		
		for (u32 i=0; i<Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
	}else 
	if(E==eStartLobby) 
	{
		LPCSTR		op		= LPCSTR	(P1);
		R_ASSERT	(g_pGamePersistent);

		g_pGamePersistent->StartLobby	( op );
	}else
	if(E==eStart) 
	{
		LPCSTR		op_server		= LPCSTR	(P1);
		LPCSTR		op_client		= LPCSTR	(P2);

		R_ASSERT	(g_pGamePersistent);
		R_ASSERT	(0==g_pGameLevel);

		g_pGamePersistent->StartNetGame	( op_server, op_client );

		xr_free						( op_server );
		xr_free						( op_client );
	}else 
	if (E==eDisconnect) 
	{
		ls_header[0] = '\0';
		ls_tip_number[0] = '\0';
		ls_tip[0] = '\0';

		if (g_pGameLevel) 
		{
			#ifndef DEDICATED_SERVER
			pConsole->Hide			();
			#endif //#ifndef DEDICATED_SERVER

			g_pGameLevel->net_Stop	();
			DEL_INSTANCE			(g_pGameLevel);
			
			#ifndef DEDICATED_SERVER
			pConsole->Show			();

			if( (FALSE == Engine.Event.Peek("KERNEL:quit")) &&(FALSE == Engine.Event.Peek("KERNEL:start")) )
			{
				pConsoleCommands->Execute("main_menu off");
				pConsoleCommands->Execute("main_menu on");
			}
			#endif //#ifndef DEDICATED_SERVER
		}
		R_ASSERT			(0!=g_pGamePersistent);
		g_pGamePersistent->Disconnect();
	}
}

static	CTimer	phase_timer		;
extern	ENGINE_API BOOL			g_appLoaded = FALSE;

void CApplication::LoadBegin	()
{
	ll_dwReference++;
	if (1==ll_dwReference)	{

		g_appLoaded			= FALSE;

#ifndef DEDICATED_SERVER
		_InitializeFont		(pFontSystem,"ui_font_letterica18_russian",0);
		m_pRender->LoadBegin();
#endif
		phase_timer.Start	();
		load_stage			= 0;
	}
}

void CApplication::LoadEnd		()
{
	ll_dwReference--;
	if (0==ll_dwReference)		{
		Msg						("* phase time: %d ms",phase_timer.GetElapsed_ms());
		Msg						("* phase cmem: %d K", Memory.mem_usage()/1024);
		pConsoleCommands->Execute("stat_memory");
		g_appLoaded				= TRUE;
	}
}

void CApplication::destroy_loading_shaders()
{
	m_pRender->destroy_loading_shaders();
}

void CApplication::LoadDraw		()
{
	if(g_appLoaded)				return;
	Device.dwFrame				+= 1;


	if(!Device.Begin () )		return;

	if(!g_dedicated_server)
		load_draw_internal			();

	Device.End					();
}

void CApplication::LoadTitleInt(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
	xr_strcpy					(ls_header, str1);
	xr_strcpy					(ls_tip_number, str2);
	xr_strcpy					(ls_tip, str3);
}

void CApplication::LoadStage()
{
	load_stage++;
	VERIFY					(ll_dwReference);
	Msg						("* phase time: %d ms",phase_timer.GetElapsed_ms());	phase_timer.Start();
	Msg						("* phase cmem: %d K", Memory.mem_usage()/1024);
	
	max_load_stage			= 14;
	LoadDraw				();
}

void CApplication::LoadSwitch	()
{
}

// Sequential
void CApplication::OnFrame	( )
{
	Engine.Event.OnFrame			();
	g_SpatialSpace->update			();
	g_SpatialSpacePhysic->update	();
	if (g_pGameLevel)				
		g_pGameLevel->SoundEvent_Dispatch	( );
}

void CApplication::Level_Append		(LPCSTR folder)
{
	string_path	N1,N2,N3,N4;
	strconcat	(sizeof(N1),N1,folder,"level");
	strconcat	(sizeof(N2),N2,folder,"level.ltx");
	strconcat	(sizeof(N3),N3,folder,"level.geom");
	strconcat	(sizeof(N4),N4,folder,"level.cform");
	if	(
		FS.exist("$game_levels$",N1)		&&
		FS.exist("$game_levels$",N2)		&&
		FS.exist("$game_levels$",N3)		&&
		FS.exist("$game_levels$",N4)	
		)
	{
		sLevelInfo			LI;
		LI.folder			= xr_strdup(folder);
		LI.name				= 0;
		Levels.push_back	(LI);
	}
}

void CApplication::Level_Scan()
{
	for (u32 i=0; i<Levels.size(); i++)
	{
		xr_free(Levels[i].folder);
		xr_free(Levels[i].name);
	}
	Levels.clear	();


	xr_vector<char*>* folder			= FS.file_list_open		("$game_levels$",FS_ListFolders|FS_RootOnly);
	
	for (u32 i=0; i<folder->size(); ++i)	
		Level_Append((*folder)[i]);
	
	FS.file_list_close		(folder);
}

void gen_logo_name(string_path& dest, LPCSTR level_name, int num)
{
	strconcat	(sizeof(dest), dest, "intro\\intro_", level_name);
	
	u32 len = xr_strlen(dest);
	if(dest[len-1]=='\\')
		dest[len-1] = 0;

	string16 buff;
	xr_strcat(dest, sizeof(dest), "_");
	xr_strcat(dest, sizeof(dest), itoa(num+1, buff, 10));
}

void CApplication::Level_Set(u32 L)
{
	if (L>=Levels.size())	return;
	FS.get_path	("$level$")->_set	(Levels[L].folder);

#ifndef DEDICATED_SERVER
	static string_path			path;

	{
		path[0]					= 0;

		int count				= 0;
		while(true)
		{
			string_path			temp2;
			gen_logo_name		(path, Levels[L].folder, count);
			if(FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds"))
				count++;
			else
				break;
		}

		if(count)
		{
			int num				= ::Random.randI(count);
			gen_logo_name		(path, Levels[L].folder, num);
		}
	}

	if(path[0])
		m_pRender->setLevelLogo	(path);
#endif //#ifndef DEDICATED_SERVER
}

int CApplication::Level_ID(LPCSTR name, LPCSTR ver, bool bSet)
{
	int result = -1;

	CLocatorAPI::archives_it it		= FS.m_archives.begin();
	CLocatorAPI::archives_it it_e	= FS.m_archives.end();
	bool arch_res					= false;

	for(;it!=it_e;++it)
	{
		CLocatorAPI::archive& A		= *it;
		if(A.hSrcFile==NULL)
		{
			LPCSTR ln = A.header->r_string("header", "level_name");
			LPCSTR lv = A.header->r_string("header", "level_ver");
			if ( 0==stricmp(ln,name) && 0==stricmp(lv,ver) )
			{
				FS.LoadArchive(A);
				arch_res = true;
			}
		}
	}

	if( arch_res )
		Level_Scan							();
	
	string256		buffer;
	strconcat		(sizeof(buffer),buffer,name,"\\");
	for (u32 I=0; I<Levels.size(); ++I)
	{
		if (0==stricmp(buffer,Levels[I].folder))	
		{
			result = int(I);	
			break;
		}
	}

	if(bSet && result!=-1)
		Level_Set(result);

	if( arch_res )
		g_pGamePersistent->OnAssetsChanged	();

	return result;
}

CInifile*  CApplication::GetArchiveHeader(LPCSTR name, LPCSTR ver)
{
	CLocatorAPI::archives_it it		= FS.m_archives.begin();
	CLocatorAPI::archives_it it_e	= FS.m_archives.end();

	for(;it!=it_e;++it)
	{
		CLocatorAPI::archive& A		= *it;

		LPCSTR ln = A.header->r_string("header", "level_name");
		LPCSTR lv = A.header->r_string("header", "level_ver");
		if ( 0==stricmp(ln,name) && 0==stricmp(lv,ver) )
		{
			return A.header;
		}
	}
	return NULL;
}

void CApplication::LoadAllArchives()
{
	if( FS.load_all_unloaded_archives() )
	{
		Level_Scan							();
		g_pGamePersistent->OnAssetsChanged	();
	}
}

#ifndef DEDICATED_SERVER
// Parential control for Vista and upper
typedef BOOL (*PCCPROC)( CHAR* ); 

BOOL IsPCAccessAllowed()
{
	CHAR szPCtrlChk[ MAX_PATH ] , szGDF[ MAX_PATH ] , *pszLastSlash;
	HINSTANCE hPCtrlChk = NULL;
	PCCPROC pctrlchk = NULL;
	BOOL bAllowed = TRUE;

	if ( ! GetModuleFileName( NULL , szPCtrlChk , MAX_PATH ) )
		return TRUE;

	if ( ( pszLastSlash = strrchr( szPCtrlChk , '\\' ) ) == NULL )
		return TRUE;

	*pszLastSlash = '\0';
	strcpy_s( szGDF , szPCtrlChk );

	strcat_s( szPCtrlChk , "\\pctrlchk.dll" );
	if ( GetFileAttributes( szPCtrlChk ) == INVALID_FILE_ATTRIBUTES )
		return TRUE;

	if ( ( pszLastSlash = strrchr( szGDF , '\\' ) ) == NULL )
		return TRUE;

	*pszLastSlash = '\0';

	strcat_s( szGDF , "\\Stalker-COP.exe" );
	if ( GetFileAttributes( szGDF ) == INVALID_FILE_ATTRIBUTES )
		return TRUE;

	if ( ( hPCtrlChk = LoadLibrary( szPCtrlChk ) ) == NULL )
		return TRUE;

	if ( ( pctrlchk = (PCCPROC) GetProcAddress( hPCtrlChk , "pctrlchk" ) ) == NULL ) {
		FreeLibrary( hPCtrlChk );
		return TRUE;
	}

	bAllowed = pctrlchk( szGDF );

	FreeLibrary( hPCtrlChk );

	return bAllowed;
}
#endif // DEDICATED_SERVER

//launcher stuff----------------------------
extern "C"{
	typedef int	 __cdecl LauncherFunc	(int);
}
HMODULE			hLauncher		= NULL;
LauncherFunc*	pLauncher		= NULL;

void InitLauncher(){
	if(hLauncher)
		return;
	hLauncher	= LoadLibrary	("xrLauncher.dll");
	if (0==hLauncher)	R_CHK	(GetLastError());
	R_ASSERT2		(hLauncher,"xrLauncher DLL raised exception during loading or there is no xrLauncher.dll at all");

	pLauncher = (LauncherFunc*)GetProcAddress(hLauncher,"RunXRLauncher");
	R_ASSERT2		(pLauncher,"Cannot obtain RunXRLauncher function from xrLauncher.dll");
};

void FreeLauncher(){
	if (hLauncher)	{ 
		FreeLibrary(hLauncher); 
		hLauncher = NULL; pLauncher = NULL; };
}


void doBenchmark(LPCSTR name)
{
	g_bBenchmark = true;
	string_path in_file;
	FS.update_path(in_file,"$app_data_root$", name);
	CInifile ini(in_file);
	int test_count = ini.line_count("benchmark");
	LPCSTR test_name,t;
	shared_str test_command;
	for(int i=0;i<test_count;++i){
		ini.r_line			( "benchmark", i, &test_name, &t);
		xr_strcpy				(g_sBenchmarkName, test_name);
		
		test_command		= ini.r_string_wb("benchmark",test_name);
		xr_strcpy			(Core.Params,*test_command);
		_strlwr_s				(Core.Params);
		
		InitInput					();
		if(i){
			//ZeroMemory(&HW,sizeof(CHW));
			//	TODO: KILL HW here!
			//  pApp->m_pRender->KillHW();
			InitEngine();
		}


		Engine.External.Initialize	( );

		xr_strcpy						(pConsoleCommands->m_ConfigFile, "user.ltx");

		if (strstr(Core.Params,"-ltx ")) 
		{
			string64				c_name;
			sscanf					(strstr(Core.Params, "-ltx ")+5, "%[^ ] ", c_name);
			xr_strcpy				(pConsoleCommands->m_ConfigFile, c_name);
		}

		Startup	 				();
	}
}

void CApplication::load_draw_internal()
{
	m_pRender->load_draw_internal(*this);
}
