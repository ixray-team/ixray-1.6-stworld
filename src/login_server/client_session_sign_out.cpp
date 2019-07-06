////////////////////////////////////////////////////////////////////////////
//	Created		: 08.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "client_session.h"

using xray::client_session;

void client_session::remove_online_user		( u32 const account_id, pcstr password )
{
	char number[11];
	_itoa							( account_id, number, 10);

	digest_type password_digest;
	fill_string_digest				( password, password_digest );

	u32 const number_length			= strlen( number );
	char const query_first[]		= "SELECT password_hash FROM stalker.accounts WHERE account_id=";
	u32 const buffer_size			= sizeof(query_first) - 1 + (number_length + 1)*sizeof(char);
	pstr const query				= static_cast<pstr>( _alloca(buffer_size) );
	memcpy							( query, query_first, sizeof(query_first) );
	memcpy							( query + sizeof(query_first) - 1, number, number_length*sizeof(char) );
	query[ buffer_size / sizeof(char) - 1 ] = 0;

	mysql_query						( &m_connection, query );
	MYSQL_RES* const result			= mysql_store_result( &m_connection );
	if ( !result ) {
		LOG_ERROR					( "error in query (null result returned)\r\n" );
		return;
	}

	if ( MYSQL_ROW const& row = mysql_fetch_row( result ) ) {
		if ( memcmp( password_digest, row[0], sizeof(password_digest) ) == 0 ) {
			LOG						( "removing user...\r\n" );
			u32 const number_length	= strlen( number );
			char const query_first[] = "DELETE FROM stalker.online_accounts WHERE account_id=";
			u32 const buffer_size	= sizeof(query_first) - 1 + (number_length + 1)*sizeof(char);
			pstr const query		= static_cast<pstr>( _alloca(buffer_size) );
			memcpy					( query, query_first, sizeof(query_first) );
			memcpy					( query + sizeof(query_first) - 1, number, number_length*sizeof(char) );
			query[ buffer_size / sizeof(char) - 1 ] = 0;

			mysql_query				( &m_connection, query );
			LOG						( "removed!\r\n" );
		}
		else
			LOG_ERROR				( "SIGN_OUT: invalid password\r\n" );
	}
	else
		LOG							( "no user with specified account id found\r\n" );

	mysql_free_result				( result );
}

void client_session::on_sign_out_password_received	(
		u32 const account_id,
		boost::system::error_code const& error_code,
		size_t const bytes_transferred
	)
{
	if ( error_code ) {
		LOG_ERROR					( "SIGN_OUT(on password received): error during reading from socket : %s\r\n", error_code.message().c_str() );
		disconnect_and_destroy_this	( );
		return;
	}

	if ( !bytes_transferred ) {
		LOG_ERROR					( "SIGN_OUT(on password received)unable to read from socket\r\n" );
		disconnect_and_destroy_this	( );
		return;
	}

	LOG								( "%d bytes have been read\r\n", bytes_transferred );
	pbyte buffer					= &m_data[0];
	// get password
	u8 const password_length		= *buffer++;
	char password[ max_password_length ];
	memcpy							( password, buffer, password_length );
	password[ password_length ]		= 0;
	buffer							+= password_length;

	remove_online_user				( account_id, password );
	disconnect_and_destroy_this		( );
}

void client_session::sign_out_on_handshaked	( u32 const account_id )
{
	LOG								( "handshaked\r\n" );
	LOG								( "waiting for password...\r\n" );

	m_ssl_stream.async_read_some	(
		boost::asio::buffer( m_data, sizeof(m_data) ),
		make_custom_alloc_handler(
			m_allocator,
			boost::bind(
				&client_session::on_sign_out_password_received,
				this,
				account_id,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}

extern long g_sign_out_count;

void client_session::process_sign_out	( u32 const session_id )
{
	BOOST_INTERLOCKED_INCREMENT		( &g_sign_out_count );

	LOG								( "signing out...\r\n" );
	char number[11];
	_itoa							( session_id, number, 10);

	u32 const number_length			= strlen( number );
	char const query_first[]		= "SELECT account_id FROM stalker.online_accounts WHERE session_id=";
	u32 const buffer_size			= sizeof(query_first) - 1 + (number_length + 1)*sizeof(char);
	pstr const query				= static_cast<pstr>( _alloca(buffer_size) );
	memcpy							( query, query_first, sizeof(query_first) );
	memcpy							( query + sizeof(query_first) - 1, number, number_length*sizeof(char) );
	query[ buffer_size / sizeof(char) - 1 ] = 0;

	mysql_query						( &m_connection, query );
	MYSQL_RES* const result			= mysql_store_result( &m_connection );
	if ( MYSQL_ROW const& row = mysql_fetch_row( result ) ) {
		LOG							( "handshaking...\r\n" );
		u32 const account_id		= atoi( row[0] );
		handshake					(
			boost::bind(
				&client_session::sign_out_on_handshaked,
				this,
				account_id
			)
		);
	}
	else {
		LOG_ERROR					( "SIGN_OUT: invalid session id\r\n" );
		disconnect_and_destroy_this	( );
	}

	mysql_free_result				( result );
}