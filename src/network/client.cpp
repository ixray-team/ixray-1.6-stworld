////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "client.h"
#include "client_error_codes.h"
#include "packet.h"

using xray::network::client;

client::client						( boost::asio::io_service& io_service ) :
	m_socket						( io_service ),
	m_packet_socket					( m_socket ),
	m_io_service					( io_service ),
	m_connection_state				( host_name_is_unresolved )
{
	m_packet_socket.set_on_error	( boost::bind(&client::on_error, this, _1, _2) );
}

client::~client						( )
{
	if ( !is_connected() )
		return;

	disconnect						( );
}

void client::start_reading			( )
{
	m_packet_socket.start_receiving	( );
}

void client::on_connected			(
		boost::system::error_code const& error_code,
		tcp::resolver::iterator iterator
	)
{
	ASSERT							( m_connection_state == connection_is_being_established );

	if ( error_code ) {
		m_connection_state			= host_name_is_unresolved;
		if ( m_on_error )
			m_on_error				( server_cannot_be_connected, error_code );
		return;
	}

	LOG								( "connection_has_been_established!\r\n" );
	m_connection_state				= connection_has_been_established;

	if ( m_on_connected )
		m_on_connected				( );

	start_reading					( );
}

void client::connect				( tcp::resolver::iterator const& iterator )
{
	m_connection_state				= connection_is_being_established;
	boost::asio::async_connect		(
		m_socket,
		iterator,
		boost::bind(
			&client::on_connected,
			this,
			_1,
			_2
		)
	);
}

void client::on_resolved			(
		tcp::resolver* const resolver,
		boost::system::error_code const& error_code,
		tcp::resolver::iterator iterator
	)
{
	ASSERT							( m_connection_state == host_name_is_being_resolved );

	if ( error_code ) {
		LOG							( "NOT host_name_has_been_resolved!\r\n" );
		LOG							( "error during host_name_is_being_resolved: %s\r\n", error_code.message().c_str() );
		++iterator;
		if ( iterator != tcp::resolver::iterator() ) {
			resolver->async_resolve	(
				*iterator,
				boost::bind(
					&client::on_resolved,
					this,
					resolver,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
				)
			);
			return;
		}

		delete						resolver;
		m_connection_state			= host_name_is_unresolved;
		LOG							( "can't resolve endpoints: %s\r\n", error_code.message().c_str() );
		LOG							( "please, try again later\r\n" );
		if ( m_on_error )
			m_on_error				( host_cannot_be_resolved, error_code );
		return;
	}

	delete							resolver;
	LOG								( "host_name_has_been_resolved!\r\n" );
	m_connection_state				= host_name_has_been_resolved;
	m_host							= iterator;
	connect							( iterator );
}

void client::resolve				( pcstr const host, u32 const host_port )
{
	LOG								( "host_name_is_being_resolved...\r\n" );

	ASSERT							( m_connection_state == host_name_is_unresolved );
	m_connection_state				= host_name_is_being_resolved;

	tcp::resolver* const resolver	= new tcp::resolver( m_io_service );

	char port[6];
	_itoa							( host_port, port, 10 );

	tcp::resolver::query query( tcp::v4(), host, port );
	resolver->async_resolve			(
		query,
		boost::bind(
			&client::on_resolved,
			this,
			resolver,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void client::connect				( pcstr const host, u16 const port )
{
	resolve							( host, port );
}

void client::disconnect				( )
{
	m_packet_socket.stop_receiving	( );
	close_connection				( );
}

void client::close_connection		( )
{
	m_socket.shutdown				( boost::asio::socket_base::shutdown_both );
	m_socket.close					( );
}

void client::send					( xray::network::packet const& packet )
{
	m_packet_socket.send			( packet );
}

void client::on_error				( xray::network::client_error_codes_enum const client_error_code, boost::system::error_code const error_code )
{
	m_connection_state				= host_name_is_unresolved;
	if ( m_on_error )
		m_on_error					( client_error_code, error_code );
}