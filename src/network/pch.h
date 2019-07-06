////////////////////////////////////////////////////////////////////////////
//	Created		: 20.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef PCH_H_INCLUDED
#define PCH_H_INCLUDED


#ifdef WIN32
#	define _WIN32_WINNT		0x0600
#endif // #ifdef WIN32

#ifdef _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS
#endif // 

#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

#include <boost/function.hpp>
#include <boost/bind.hpp>

typedef char				s8;
typedef unsigned char		u8;

typedef short				s16;
typedef unsigned short		u16;

typedef int					s32;
typedef unsigned int		u32;

typedef long long			s64;
typedef unsigned long long	u64;

typedef void*				pvoid;
typedef void const*			pcvoid;

typedef char*				pstr;
typedef char const*			pcstr;

typedef u8*					pbyte;
typedef u8 const*			pcbyte;

#ifndef NDEBUG
	namespace xray {
		namespace debug {
			template < typename T >
			inline T const& identity	( T const& value )
			{
				return	value;
			}
		} // namespace debug
	} // namespace xray

#	define ASSERT( expression, ... )	\
		if ( !xray::debug::identity(expression) ) \
			__debugbreak	( ); \
		else (void)0
#else // #ifndef NDEBUG
#	define ASSERT( ... )	(void)0
#endif // #ifndef NDEBUG

#ifndef NDEBUG
#	define XRAY_DEBUG_LOGGING_ENABLED
#endif // #ifndef NDEBUG

#ifdef XRAY_DEBUG_LOGGING_ENABLED
#	define	LOG				printf
#else // #ifdef XRAY_DEBUG_LOGGING_ENABLED
#	define	LOG(...)		(void)0
#endif // #ifdef XRAY_DEBUG_LOGGING_ENABLED

#define LOG_INFO			LOG

#define XRAY_ERROR_LOGGING_ENABLED

#ifdef XRAY_ERROR_LOGGING_ENABLED
#	define	LOG_ERROR		printf
#else // #ifdef XRAY_DEBUG_LOGGING_ENABLED
#	define	LOG_ERROR(...)	(void)0
#endif // #ifdef XRAY_DEBUG_LOGGING_ENABLED

#define FATAL( ... )		ASSERT( false, __VA_ARGS__ )

namespace xray {

	template < typename T >
	inline T identity	( T const& value ) { return value; }

namespace detail {
	inline void	unreferenced_parameter_helper ( ... ) {}
} // namespace detail
} // namespace xray

#define XRAY_UNREFERENCED_PARAMETER( parameter ) (void)(&parameter)

#define XRAY_UNREFERENCED_PARAMETERS( ... ) \
	if ( xray::identity(false) ) { xray::detail::unreferenced_parameter_helper(__VA_ARGS__); } else (void)0

#define XRAY_MAKE_STRING_HELPER(a)		#a
#define XRAY_MAKE_STRING(a)				XRAY_MAKE_STRING_HELPER(a)

#if defined(_MSC_VER)
#	define XRAY_PRINTF_SPEC_LONG_LONG( digits )			"%" XRAY_MAKE_STRING( digits ) "I64d"
#	define XRAY_PRINTF_SPEC_LONG_LONG_HEX( digits )		"%" XRAY_MAKE_STRING( digits ) "I64x"
#else // #if defined(_MSC_VER)
#	define XRAY_PRINTF_SPEC_LONG_LONG( digits )			"%" XRAY_MAKE_STRING( digits ) "lld"
#	define XRAY_PRINTF_SPEC_LONG_LONG_HEX( digits )		"%" XRAY_MAKE_STRING( digits ) "llx"
#endif // #if defined(_MSC_VER)

namespace xray {
namespace threading {

u32 current_thread_id	( );

} // namespace threading
} // namespace xray

#endif // #ifndef PCH_H_INCLUDED