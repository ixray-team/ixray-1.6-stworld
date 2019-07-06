////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "doug_lea_allocator.h"

#define USE_DL_PREFIX
#include "ptmalloc3/malloc-2.8.3.h"

namespace xray {
namespace memory {

doug_lea_allocator::doug_lea_allocator	( ) :
	m_arena						( 0 ),
	m_arena_start				( 0 ),
	m_arena_end					( 0 ),
	m_arena_id					( 0 ),
	m_user_thread_id			( xray::threading::current_thread_id( ) )
{
}

static char __stdcall out_of_memory_with_crash	( mspace const space, void const* const parameter, int const first_time )
{
	XRAY_UNREFERENCED_PARAMETERS( parameter, space, first_time );

	LOG_ERROR					( "out of memory in arena \"%s\"", (( doug_lea_allocator* )parameter)->arena_id( ) );
	FATAL						( "not enough memory for arena [%s]", (( doug_lea_allocator* )parameter)->arena_id( ) );
	return						0;
}

static char __stdcall out_of_memory_silent		( mspace const space, void const* const parameter, int const first_time )
{
	XRAY_UNREFERENCED_PARAMETER	( parameter );
	XRAY_UNREFERENCED_PARAMETER	( space );
	if ( first_time )
		return					0;

	FATAL						( "not enough memory for arena [%s]", (( doug_lea_allocator* )parameter)->arena_id( ) );
	return						1;
}

void doug_lea_allocator::user_thread_id	( u32 user_thread_id ) const
{
	m_user_thread_id			= user_thread_id;
}

void doug_lea_allocator::user_current_thread_id	( ) const
{
	user_thread_id				( threading::current_thread_id( ) );
}

void doug_lea_allocator::initialize_impl( pvoid arena, u64 arena_size, pcstr arena_id )
{
	XRAY_UNREFERENCED_PARAMETER	( arena_id );

	ASSERT						( !m_arena, "arena is not initialized or is initialized more than once" );
	m_arena						= create_xray_mspace_with_base( arena, arena_size, 0, &out_of_memory_with_crash, (pcvoid)this );
}

void doug_lea_allocator::initialize	( pvoid arena, u64 size, pcstr arena_id )
{
	ASSERT						( !initialized ( ) );

	m_arena_id					= arena_id;
	m_arena_start				= arena;
	m_arena_end					= (u8*)arena + size;

	initialize_impl				( arena, size, arena_id );
}

void doug_lea_allocator::finalize_impl	( )
{
	ASSERT						( m_arena, "arena is not initialized or is initialized more than once" );
	destroy_mspace				( m_arena );
}

void doug_lea_allocator::finalize	( )
{
	ASSERT						( initialized ( ) );

	finalize_impl				( );

	m_arena_start				= 0;
	m_arena_end					= 0;
	m_arena_id					= 0;
}

inline bool doug_lea_allocator::initialized	( ) const
{
	if ( !m_arena_start )
		return					( false );

	u32 const current_thread_id	= xray::threading::current_thread_id ( );
	XRAY_UNREFERENCED_PARAMETER	( current_thread_id );
	ASSERT						( current_thread_id == m_user_thread_id	);
	return						( true );
}

void doug_lea_allocator::dump_statistics( ) const
{
	u64 const total_size		= this->total_size( );
	u64 const allocated_size	= this->allocated_size( );

	LOG_INFO					( "---------------memory stats for arena [%s]---------------", m_arena_id );
	LOG_INFO					( "used: " XRAY_PRINTF_SPEC_LONG_LONG(10) " (%6.2f%%)", allocated_size, total_size == 0.f ? 0.f : float(allocated_size)/float(total_size)*100.f );
	LOG_INFO					( "free: " XRAY_PRINTF_SPEC_LONG_LONG(10) " (%6.2f%%)", total_size - allocated_size, total_size == 0.f ? 0.f : float(total_size - allocated_size)/float(total_size)*100.f );
	LOG_INFO					( 
		m_arena_start
		?
			"size: " XRAY_PRINTF_SPEC_LONG_LONG(10)
			", start address: 0x" XRAY_PRINTF_SPEC_LONG_LONG_HEX(09)
			", end address: 0x" XRAY_PRINTF_SPEC_LONG_LONG_HEX(09)
		: 
			"size: " XRAY_PRINTF_SPEC_LONG_LONG(10),
		total_size,
		(u64)*(size_t*)&m_arena_start,
		(u64)*(size_t*)&m_arena_end
	);
}

size_t doug_lea_allocator::usable_size ( pvoid pointer ) const 
{
	return						usable_size_impl( pointer );
}

void doug_lea_allocator::call_initialize_impl	( doug_lea_allocator& allocator, pvoid arena, u64 size, pcstr arena_id )
{
	allocator.initialize_impl	( arena, size, arena_id );
}

void doug_lea_allocator::call_finalize_impl		( doug_lea_allocator& allocator )
{
	allocator.finalize_impl		( );
}

void doug_lea_allocator::copy			( doug_lea_allocator const& allocator )
{
	m_arena_start				= allocator.m_arena_start;
	m_arena_end					= allocator.m_arena_end;
	m_arena_id					= allocator.m_arena_id;
}

pvoid doug_lea_allocator::malloc_impl	( size_t size )
{
	ASSERT						( initialized ( ) );
	size_t const real_size		= needed_size( size );
	pvoid const result			= xray_mspace_malloc( m_arena, real_size );
	if ( !result )
	{
		return					0;
	}
	return						( on_malloc	( result, size, 0, "" ) );
}

pvoid doug_lea_allocator::realloc_impl	( pvoid pointer, size_t new_size )
{
	ASSERT						( initialized ( ) );
	if ( !new_size ) {
		free_impl				( pointer );
		return					( 0 );
	}

	size_t const previous_size	= pointer ? usable_size( pointer ) : 0;
	if ( pointer )
		on_free					( pointer, false );

	size_t const real_size		= needed_size( new_size );
	pvoid const result			= xray_mspace_realloc( m_arena, pointer, real_size );
	return
		on_malloc(
			result,
			new_size,
			previous_size,
			""
		);
}

void doug_lea_allocator::free_impl		( pvoid pointer )
{
	ASSERT						( initialized ( ) );

	if ( !pointer )
		return;

	on_free						( pointer );

	xray_mspace_free			( m_arena, pointer );
}

pvoid doug_lea_allocator::call_malloc	( size_t size )
{
	return						( malloc_impl( size ) );
}

pvoid doug_lea_allocator::call_realloc	( pvoid pointer, size_t new_size )
{
	return						( realloc_impl( pointer, new_size ) );
}

void doug_lea_allocator::call_free		( pvoid pointer )
{
	free_impl					( pointer );
}

size_t doug_lea_allocator::total_size	( ) const
{
	u32 const owner_thread_id	= m_user_thread_id;
	user_current_thread_id		( );

	ASSERT						( initialized ( ) );
	size_t const result			= xray_mspace_mallinfo( m_arena ).usmblks;

	user_thread_id				( owner_thread_id );

	return						( result );
}

size_t doug_lea_allocator::usable_size_impl	( pvoid pointer ) const
{
	return						xray_mspace_usable_size( pointer );
}

#include "ptmalloc3/malloc-private.h"

size_t doug_lea_allocator::allocated_size( ) const
{
	u32 const owner_thread_id	= m_user_thread_id;
	user_current_thread_id		( );

	ASSERT						( initialized ( ) );
	u32 result					= (u32)xray_mspace_mallinfo( m_arena ).uordblks;

	m_user_thread_id			= owner_thread_id;

	u32 const min_size			= pad_request( sizeof( malloc_state ) );
	ASSERT						( result >= min_size );
	result						-= min_size;
	return						( result );
}

} // namespace memory
} // namespace xray