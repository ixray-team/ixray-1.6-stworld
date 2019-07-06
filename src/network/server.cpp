////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "server.h"
#include "client_session.h"

using xray::network::server;

server::server						( boost::asio::io_service& io_service ) :
	m_io_service					( io_service ),
	m_client_acceptor				( io_service ),
	m_first_client					( 0 ),
	m_max_connections_count			( 0 ),
	m_connections_count				( 0 )
{
}

server::~server						( )
{
	for ( ; m_first_client; ) {
		client_session* const next_client_session	= m_first_client->next; 
		delete						m_first_client;
		m_first_client				= next_client_session;
	}
}

void server::delete_client_session	( xray::network::client_session* const client_session )
{
	delete							client_session;

	if ( m_connections_count-- >= m_max_connections_count ) {
		if ( m_connections_count < m_max_connections_count )
			start_client_acception	( );
	}
}

void server::process_client			(
		xray::network::client_session* const new_client,
		boost::system::error_code const& error
	)
{
	if ( !error && ( !m_on_connected || (m_on_connected(*new_client) == permit_connection )) ) {
		ASSERT						( !new_client->next );
		new_client->next			= m_first_client;
		m_first_client				= new_client;
		new_client->start			( );
		LOG							( "new client connected\r\n" );
	}
	else
		delete_client_session		( new_client );

	start_client_acception			( );
}

void server::start_client_acception	( )
{
	client_session* const new_client = new client_session( m_io_service, *this );
	m_client_acceptor.async_accept	(
		new_client->socket(),
		make_custom_alloc_handler(
			m_socket_allocator,
			boost::bind(
				&server::process_client,
				this,
				new_client,
				boost::asio::placeholders::error
			)
		)
	);
}

void server::accept_connections		( u16 const port, u32 const max_connections_count )
{
	ASSERT							( !m_first_client );
	m_client_acceptor.open			( tcp::v4() );
	m_client_acceptor.bind			( tcp::endpoint( tcp::v4(), port ) );
	m_client_acceptor.listen		( );

	m_max_connections_count			= max_connections_count;

	start_client_acception			( );
}

void server::send_broadcast			( xray::network::packet const& packet )
{
}

void server::set_on_connected		( on_connected_functor_type const& functor )
{
	m_on_connected					= functor;
}

void server::set_on_disconnected	( on_disconnected_functor_type const& functor )
{
	m_on_disconnected				= functor;
}