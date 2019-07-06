////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_MEMORY_DOUG_LEA_ALLOCATOR_H_INCLUDED
#define XRAY_MEMORY_DOUG_LEA_ALLOCATOR_H_INCLUDED

namespace xray {
namespace memory {

class doug_lea_allocator : private boost::noncopyable {
public:
					doug_lea_allocator		( );
	inline			~doug_lea_allocator		( ) { }
			void	initialize				( pvoid arena, u64 size, pcstr arena_id );
			void	finalize				( );
			void	dump_statistics			( ) const;

			pvoid	malloc_impl				( size_t size );
			pvoid	realloc_impl			( pvoid pointer, size_t new_size );
			void	free_impl				( pvoid pointer );

			bool	initialized				( ) const;
			void	user_thread_id			( u32 user_thread_id ) const;
			void	user_current_thread_id	( ) const;
	inline	pcstr	arena_id				( ) const { return m_arena_id; }
			size_t	total_size				( ) const;
			size_t	allocated_size			( ) const;
			size_t	usable_size				( pvoid pointer ) const;

protected:
			pvoid	call_malloc				( size_t size_t );
			pvoid	call_realloc			( pvoid pointer, size_t new_size );
			void	call_free				( pvoid pointer );
			size_t	usable_size_impl		( pvoid pointer ) const;

protected:
			void	copy					( doug_lea_allocator const& allocator );
	inline	pvoid	on_malloc				( pvoid buffer, size_t buffer_size, size_t previous_size, pcstr description ) const { XRAY_UNREFERENCED_PARAMETER(buffer_size); XRAY_UNREFERENCED_PARAMETER(previous_size); XRAY_UNREFERENCED_PARAMETER(description); return buffer; }
	inline	void	on_free					( pvoid buffer, bool can_clear = true ) const { XRAY_UNREFERENCED_PARAMETER(can_clear); XRAY_UNREFERENCED_PARAMETER(buffer); }
	inline	size_t	needed_size				( size_t const size) const { ASSERT( size ); return size; }

protected:
			void	call_initialize_impl	( doug_lea_allocator& allocator, pvoid arena, u64 size, pcstr arena_id );
			void	call_finalize_impl		( doug_lea_allocator& allocator );

private:
			pvoid	on_malloc_check_magic	( pvoid buffer, size_t buffer_size, size_t previous_size, pcstr description ) const;
			void	on_free_check_magic		( pvoid buffer, bool clear_with_magic ) const;
			size_t	needed_size_for_magic	( size_t const size) const;

			void	initialize_impl			( pvoid arena, u64 size, pcstr arena_id );
			void	finalize_impl			( );

private:
	pvoid		m_arena_start;
	pvoid		m_arena_end;
	pcstr		m_arena_id;
	pvoid		m_arena;
	mutable u32	m_user_thread_id;
}; // class doug_lea_allocator

} // namespace memory
} // namespace xray

#endif // #ifndef XRAY_MEMORY_DOUG_LEA_ALLOCATOR_H_INCLUDED