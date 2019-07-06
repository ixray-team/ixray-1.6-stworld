////////////////////////////////////////////////////////////////////////////
//	Created		: 08.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "client_session.h"
#include "../login_structures.h"

using xray::client_session;

void client_session::on_sign_up_result_sent	(
		boost::system::error_code const& error_code,
		size_t const bytes_transferred
	)
{
	(void)error_code;
	(void)bytes_transferred;

	disconnect_and_destroy_this		( );
}

void client_session::on_sign_up_info_received	(
		std::string const& account_name,
		boost::system::error_code const& error_code,
		size_t const bytes_transferred
	)
{
	if ( error_code ) {
		LOG_ERROR					( "SIGN_UP(on sign up info received): error during reading from socket : %s\r\n", error_code.message().c_str() );
		disconnect_and_destroy_this	( );
		return;
	}

	if ( !bytes_transferred ) {
		LOG_ERROR					( "SIGN_UP(on sign up info received): unable to read from socket\r\n" );
		disconnect_and_destroy_this	( );
		return;
	}

	LOG								( "%d bytes have been read\r\n", bytes_transferred );
	sign_up_info sign_up_info;

	strcpy							( sign_up_info.account_name, account_name.c_str() );

	pbyte buffer					= &m_data[0];

	// get password
	u8 const password_length		= *buffer++;
	memcpy							( sign_up_info.password, buffer, password_length );
	sign_up_info.password[ password_length ]		= 0;
	buffer							+= password_length;

	// get email
	u8 const email_length			= *buffer++;
	memcpy							( sign_up_info.email, buffer, email_length );
	sign_up_info.email[ email_length ]			= 0;
	buffer							+= email_length;

	add_new_user					( sign_up_info );

	m_data[0]						= (u8)sign_up_successful_message_type;
	boost::asio::async_write		(
		m_ssl_stream,
		boost::asio::buffer( m_data, 1*sizeof(u8) ),
		make_custom_alloc_handler(
			m_allocator,
			boost::bind (
				&client_session::on_sign_up_result_sent,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}

void client_session::sign_up_on_handshaked	( std::string const& account_name )
{
	m_ssl_stream.async_read_some	(
		boost::asio::buffer( m_data, sizeof(m_data) ),
		make_custom_alloc_handler(
			m_allocator,
			boost::bind(
				&client_session::on_sign_up_info_received,
				this,
				account_name,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}

void client_session::on_sign_up_more_info_sent	(
		std::string const& account_name_string,
		boost::system::error_code const& error_code,
		size_t const bytes_transferred
	)
{
	if ( error_code ) {
		LOG_ERROR					( "SIGN_UP(on more info sent): error during writing to socket: %s\r\n", error_code.message().c_str() );
		disconnect_and_destroy_this	( );
		return;
	}

	if ( !bytes_transferred ) {
		LOG_ERROR					( "SIGN_UP(on more info sent): unable to write to socket\r\n" );
		disconnect_and_destroy_this	( );
		return;
	}

	handshake						(
		boost::bind(
			&client_session::sign_up_on_handshaked,
			this,
			account_name_string
		)
	);
}

extern long g_sign_up_count;

void client_session::process_sign_up		( )
{
	BOOST_INTERLOCKED_INCREMENT		( &g_sign_up_count );

	char account_name[ max_account_name_length ];
	pbyte buffer					= &m_data[1];
	u8 const account_name_length	= *buffer++;
	memcpy							( account_name, buffer, account_name_length );
	account_name[ account_name_length ]	= 0;
	buffer							+= account_name_length;

	char const query_first[]		= "SELECT account_id,password_hash FROM stalker.accounts WHERE account_name = '";
	char const query_last[]			= "\\0'";
	pstr const query				= static_cast<pstr>( _alloca( sizeof(query_first) - 1 + sizeof(query_last) - 1 + (account_name_length + 1)*sizeof(char) ) );
	memcpy							( query, query_first, sizeof(query_first) );
	memcpy							( query + sizeof(query_first) - 1, account_name, account_name_length*sizeof(char) );
	memcpy							( query + sizeof(query_first) - 1 + account_name_length*sizeof(char), query_last, sizeof(query_last) );

	mysql_query						( &m_connection, query );
	MYSQL_RES* const result			= mysql_store_result( &m_connection );
	if ( MYSQL_ROW const& row = mysql_fetch_row( result ) ) {
		send_message				( occupied_user_name_message_type, insecure, destroy_after_message_sent );
		mysql_free_result			( result );
		return;
	}

	mysql_free_result				( result );
	
	m_data[0]						= (u8)send_sign_up_info_message_type;
	boost::asio::async_write		(
		m_socket,
		boost::asio::buffer( m_data, 1*sizeof(u8) ),
		make_custom_alloc_handler(
			m_allocator,
			boost::bind (
				&client_session::on_sign_up_more_info_sent,
				this,
				std::string(account_name),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		)
	);
}