////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_INTRUSIVE_SPSC_QUEUE_H_INCLUDED
#define XRAY_NETWORK_INTRUSIVE_SPSC_QUEUE_H_INCLUDED

namespace xray {

namespace threading {
	typedef long atomic32_type;
} // namespace threading

namespace network {

template < typename T, typename BaseWithMember, T* BaseWithMember::*MemberNext >
class intrusive_spsc_queue {
public:
	typedef T			value_type;
	typedef value_type*	pointer_type;

public:
	inline			intrusive_spsc_queue	( );
	inline			~intrusive_spsc_queue	( );

	inline	void	push_back				( T* const value );
	inline	T*		pop_front				( T*& item_to_delete );
	inline	bool	empty					( ) const;

	inline	void	set_push_thread_id		( );
	inline	void	push_null_node			( T* const null_node );

	inline	void	set_pop_thread_id		( );
	inline	T*		pop_null_node			( );

	inline	T*		null_node				( ) const;
	inline	void	clear_push_thread_id	( ) { m_push_thread_id = threading::atomic32_value_type(-1); }

	inline	threading::atomic32_type	push_thread_id	( ) const { return m_push_thread_id; }
	inline	threading::atomic32_type	pop_thread_id	( ) const { return m_pop_thread_id; }

private:
	enum {
		cache_line_pad_size	=
			64 - 1*sizeof(T*)
			- 2*sizeof(threading::atomic32_type)
		,
	};

private:
	T*							m_head;
	threading::atomic32_type	m_push_thread_id;
	threading::atomic32_type	m_pop_thread_id;
	char						m_cache_line_pad[ cache_line_pad_size ];
	T*							m_tail;
}; // class intrusive_spsc_queue

} // namespace network
} // namespace xray

#include "intrusive_spsc_queue_inline.h"

#endif // #ifndef XRAY_NETWORK_INTRUSIVE_SPSC_QUEUE_H_INCLUDED