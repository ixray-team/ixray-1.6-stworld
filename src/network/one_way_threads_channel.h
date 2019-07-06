////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_ONE_WAY_THREADS_CHANNEL_H_INCLUDED
#define XRAY_NETWORK_ONE_WAY_THREADS_CHANNEL_H_INCLUDED

#include <boost/type_traits/is_same.hpp>
#include "doug_lea_allocator.h"

namespace xray {
namespace network {

template < typename ForwardQueueType, typename BackwardQueueType >
class one_way_threads_channel : private boost::noncopyable {
public:
	typedef typename ForwardQueueType::pointer_type	pointer_type;
	enum {
		are_queues_use_the_same_pointer_type = boost::is_same< pointer_type, typename BackwardQueueType::pointer_type >::value,
	};

public:
	inline					one_way_threads_channel		(
								memory::doug_lea_allocator& owner_allocator
							);

	// owner functionality
	// must be called from the owner thread ONLY
public:
	inline	void			owner_initialize			(
								pointer_type const forward_queue_initial_value,
								pointer_type const backward_queue_initial_value
							);
	inline	void			owner_finalize				( );
	template < typename DeferredValuesContainerType >
	inline	void			owner_finalize				( DeferredValuesContainerType& container );
	inline	void			owner_push_back				( pointer_type value );
	inline	void			owner_delete_processed_items( );
	inline	memory::doug_lea_allocator&	owner_allocator		( ) const;

private:
	struct one_way_threads_channel_default_pop_front_predicate {
		template < typename T >
		inline bool operator ( )	( T const& ) const { return true; }
	}; // struct one_way_threads_channel_default_pop_front_predicate

	// user functionality
	// must be called from the user thread ONLY
public:
	inline	void			user_initialize				( );
	inline	bool			user_is_queue_empty			( ) const;
	inline	pointer_type	user_pop_front				( );
	template < typename PredicateType >
	inline	pointer_type	user_pop_front				( PredicateType const& predicate );
	inline	void			user_delete_deffered_value	( pointer_type value_to_delete );

private:
	inline	void			delete_value				( pointer_type value );
	inline	pointer_type owner_get_next_value_to_delete	( );

private:
	ForwardQueueType			m_forward_queue;
	BackwardQueueType			m_backward_queue;
	memory::doug_lea_allocator& m_owner_allocator;
}; // class one_way_threads_channel

} // namespace network
} // namespace xray

#include "one_way_threads_channel_inline.h"

#endif // #ifndef ENGINE_NETWORK_ONE_WAY_THREADS_CHANNEL_H_INCLUDED