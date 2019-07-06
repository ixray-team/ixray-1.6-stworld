////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "network_world.h"

using xray::network::network_world;

network_world::network_world			(
		xray::memory::doug_lea_allocator& responses_allocator,
		xray::memory::doug_lea_allocator& orders_allocator
	) :
	m_channel							( responses_allocator, orders_allocator )
{
}

static void empty_function ( ) { }

void network_world::owner_initialize	( )
{
	m_channel.orders.user_initialize	( );
	m_channel.responses.owner_initialize( new network_functor_response( &empty_function ), new network_functor_response( &empty_function ) );
}

void network_world::user_initialize		( )
{
	m_channel.orders.owner_initialize	( new network_functor_order( &empty_function ), new network_functor_order( &empty_function ) );
	m_channel.responses.user_initialize	( );
}

void network_world::process_orders		( )
{
	while ( !m_channel.orders.user_is_queue_empty() ) {
		network_order* const order		= m_channel.orders.user_pop_front( );
		order->execute					( );
	}
}

void network_world::process_responses	( )
{
	while ( !m_channel.responses.user_is_queue_empty() ) {
		network_response* const response = m_channel.responses.user_pop_front( );
		response->execute				( );
	}
}

void network_world::add_order			( xray::network::network_order* order )
{
	m_channel.orders.owner_push_back	( order );
}

void network_world::add_response		( xray::network::network_response* response )
{
	m_channel.responses.owner_push_back	( response );
}