////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_NETWORK_TWO_WAY_THREADS_CHANNEL_H_INCLUDED
#define ENGINE_NETWORK_TWO_WAY_THREADS_CHANNEL_H_INCLUDED

#include "intrusive_spsc_queue.h"
#include "one_way_threads_channel.h"

namespace xray {
namespace network {

class network_order {
public:
	virtual			~network_order			( ) { }
	virtual	void	execute					( ) = 0;

public:
	network_order*	m_next_for_orders;
}; // class network_order

class network_functor_order : public network_order {
public:
	typedef boost::function< void ( ) >	functor_type;

public:
	inline			network_functor_order	( functor_type const& functor ) : m_functor( functor ) { }
	virtual	void	execute					( ) { m_functor( ); }

private:
	functor_type	m_functor;
}; // class network_functor_order

class network_response {
public:
	virtual			~network_response		( ) { }
	virtual	void	execute					( ) = 0;

public:
	network_response* m_next;
}; // class network_response

class network_functor_response : public network_response {
public:
	typedef boost::function< void ( ) >	functor_type;

public:
	inline			network_functor_response( functor_type const& functor ) : m_functor( functor ) { }
	virtual	void	execute					( ) { m_functor( ); }

private:
	functor_type	m_functor;
}; // class network_functor_response

typedef intrusive_spsc_queue< network_order, network_order, &network_order::m_next_for_orders >		network_orders_queue_type;
typedef intrusive_spsc_queue< network_response, network_response, &network_response::m_next >		network_responses_queue_type;

typedef one_way_threads_channel<
	network_responses_queue_type,
	network_responses_queue_type
>	responses_channel_type;

typedef one_way_threads_channel<
	network_orders_queue_type,
	network_orders_queue_type
>	orders_channel_type;

struct two_way_threads_channel : private boost::noncopyable {
	inline two_way_threads_channel	(
			memory::doug_lea_allocator& responses_allocator,
			memory::doug_lea_allocator& orders_allocator
		) :
		responses	( responses_allocator ),
		orders		( orders_allocator )
	{
	}

	responses_channel_type	responses;
	orders_channel_type		orders;
}; // struct two_way_threads_channel

} // namespace network
} // namespace xray

#endif // #ifndef ENGINE_NETWORK_TWO_WAY_THREADS_CHANNEL_H_INCLUDED