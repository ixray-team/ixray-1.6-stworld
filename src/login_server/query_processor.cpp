////////////////////////////////////////////////////////////////////////////
//	Created		: 07.02.2012
//	Author		: Tetyana Meleshchenko
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "query_processor.h"
#include <boost/uuid/sha1.hpp>

namespace xray {

query_processor::query_processor( MYSQL& connection, boost::program_options::variables_map const& options ) :
	m_connection				( connection ),
	m_account_id				( u32(-1) ),
	m_invalid_attempts_count	( u32(-1) ),
	m_squad_leader_id			( u32(-1) ),
	m_session_id				( u32(-1) ),
 	m_no_ping_interval			( 0 )
{
	if ( !mysql_thread_safe() ) {
		LOG_ERROR				( "MySQL library is not thread safe!\r\n" );
	}

	mysql_init					( &m_connection );

	bool const sql_server_connection_result	=
		mysql_real_connect		(
			&m_connection,
			options["database_server"].as< std::string >().c_str(),
			options["database_user"].as< std::string >().c_str(),
			options["database_password"].as< std::string >().c_str(),
			options["database_name"].as< std::string >().c_str(),
			0,
			NULL,
			0
		) != NULL;
	if ( !sql_server_connection_result )
	{
		char const* const error_message = mysql_error(&m_connection);
		LOG_ERROR				( "can't connect to stalker database:%s\r\nexiting...\r\n", error_message );
		return;
	}
	
	m_add_new_user				= mysql_stmt_init( &connection );
	m_update_sign_in_stats		= mysql_stmt_init( &connection );
	m_add_new_online_user		= mysql_stmt_init( &connection );
	m_set_last_activity_time	= mysql_stmt_init( &connection );
	m_disconnect_inactive_users = mysql_stmt_init( &connection );

	char const add_new_query[]	= "INSERT INTO stalker.accounts(account_name,password_hash,email,next_sign_in_attempt_time) VALUES(?,?,?,?)";
	mysql_stmt_prepare			( m_add_new_user, add_new_query, sizeof( add_new_query ) );

	char const update_query[]	= "UPDATE stalker.accounts SET invalid_sign_in_attempts_count=?,next_sign_in_attempt_time=? WHERE account_id=?";
	mysql_stmt_prepare			( m_update_sign_in_stats, update_query, sizeof( update_query ) );

	char const add_online_query[] = "INSERT INTO stalker.online_accounts(account_id,ip_address,squad_leader_id,last_activity_time) VALUES(?,?,?,?)";
	mysql_stmt_prepare			( m_add_new_online_user, add_online_query, sizeof( add_online_query ) );

	char const update_time_query[] = "UPDATE stalker.online_accounts SET last_activity_time=? WHERE session_id=?";
	mysql_stmt_prepare			( m_set_last_activity_time, update_time_query, sizeof( update_time_query ) );

	char const disconnect_query[] = "DELETE FROM stalker.online_accounts WHERE last_activity_time < ?";
	mysql_stmt_prepare			( m_disconnect_inactive_users, disconnect_query, sizeof( disconnect_query ) );

	memset						( m_add_new_user_bind, 0, sizeof( m_add_new_user_bind ) );
	memset						( m_update_sign_in_stats_bind, 0, sizeof( m_update_sign_in_stats_bind ) );
	memset						( m_add_new_online_user_bind, 0, sizeof( m_add_new_online_user_bind ) );
	memset						( m_set_last_activity_time_bind, 0, sizeof( m_set_last_activity_time_bind ) );
	memset						( m_disconnect_inactive_users_bind, 0, sizeof( m_disconnect_inactive_users_bind ) );
}

query_processor::~query_processor	( )
{
	mysql_stmt_close			( m_add_new_user );
	mysql_stmt_close			( m_update_sign_in_stats );
	mysql_stmt_close			( m_add_new_online_user );
	mysql_stmt_close			( m_set_last_activity_time );
	mysql_stmt_close			( m_disconnect_inactive_users );
}

typedef u32						digest_type[5];

static void fill_string_digest	( pcstr const string, digest_type& digest )
{
	boost::uuids::detail::sha1	sha1;
	sha1.process_block			( string, string + strlen(string) );
	sha1.get_digest				( digest );
}

void query_processor::add_new_account		( )
{
	m_add_new_user_bind[0].buffer_type		= MYSQL_TYPE_VARCHAR;
	m_add_new_user_bind[0].buffer			= &m_sign_up_info.account_name[0];
	m_add_new_user_bind[0].buffer_length	= strlen( m_sign_up_info.account_name ) + 1;

	digest_type								password_digest;
	fill_string_digest						( m_sign_up_info.password, password_digest );
	m_add_new_user_bind[1].buffer_type		= MYSQL_TYPE_BLOB;
	m_add_new_user_bind[1].buffer			= &password_digest[0];
	m_add_new_user_bind[1].buffer_length	= sizeof( password_digest );

	m_add_new_user_bind[2].buffer_type		= MYSQL_TYPE_VARCHAR;
	m_add_new_user_bind[2].buffer			= &m_sign_up_info.email[0];
	m_add_new_user_bind[2].buffer_length	= strlen( m_sign_up_info.email ) + 1;

	boost::posix_time::ptime const& now		= boost::posix_time::microsec_clock::local_time();
	MYSQL_TIME								current_time;
	current_time.time_type					= MYSQL_TIMESTAMP_DATETIME;
	current_time.year						= now.date().year();
	current_time.month						= now.date().month();
	current_time.day						= now.date().day();
	current_time.hour						= now.time_of_day().hours();
	current_time.minute						= now.time_of_day().minutes();
	current_time.second						= now.time_of_day().seconds();
	current_time.second_part				= u32( now.time_of_day().total_milliseconds() - now.time_of_day().total_seconds() * 1000 );
	current_time.neg						= false;
	
	m_add_new_user_bind[3].buffer_type		= MYSQL_TYPE_DATETIME;
	m_add_new_user_bind[3].buffer			= &current_time;
	m_add_new_user_bind[3].buffer_length	= sizeof( current_time );

	mysql_stmt_bind_param					( m_add_new_user, m_add_new_user_bind );
	mysql_stmt_execute						( m_add_new_user );
}

void query_processor::set_sign_up_info		( sign_up_info const& info )
{
	m_sign_up_info							= info;
	add_new_account							( );
}

void query_processor::update_sign_in_stats	( )
{
	m_update_sign_in_stats_bind[0].buffer_type		= MYSQL_TYPE_LONG;
	m_update_sign_in_stats_bind[0].buffer			= &m_invalid_attempts_count;
	m_update_sign_in_stats_bind[0].buffer_length	= sizeof( m_invalid_attempts_count );

	MYSQL_TIME						current_time;
	current_time.time_type			= MYSQL_TIMESTAMP_DATETIME;
	current_time.year				= m_next_attempt_time.date().year();
	current_time.month				= m_next_attempt_time.date().month();
	current_time.day				= m_next_attempt_time.date().day();
	current_time.hour				= m_next_attempt_time.time_of_day().hours();
	current_time.minute				= m_next_attempt_time.time_of_day().minutes();
	current_time.second				= m_next_attempt_time.time_of_day().seconds();
	current_time.second_part		= u32( m_next_attempt_time.time_of_day().total_milliseconds() - m_next_attempt_time.time_of_day().total_seconds() * 1000 );
	current_time.neg				= false;
	
	m_update_sign_in_stats_bind[1].buffer_type		= MYSQL_TYPE_DATETIME;
	m_update_sign_in_stats_bind[1].buffer			= &current_time;
	m_update_sign_in_stats_bind[1].buffer_length	= sizeof(current_time);

	m_update_sign_in_stats_bind[2].buffer_type		= MYSQL_TYPE_LONG;
	m_update_sign_in_stats_bind[2].buffer			= &m_account_id;
	m_update_sign_in_stats_bind[2].buffer_length	= sizeof( m_account_id );

	mysql_stmt_bind_param			( m_update_sign_in_stats, m_update_sign_in_stats_bind );
	mysql_stmt_execute				( m_update_sign_in_stats );
}

void query_processor::set_next_attempt_time	( u32 account_id, u32 invalid_attempts_count, boost::posix_time::ptime const& next_attempt_time )
{
	m_account_id					= account_id;
	m_invalid_attempts_count		= invalid_attempts_count;
	m_next_attempt_time				= next_attempt_time;
	update_sign_in_stats			( );
}

u32 query_processor::add_new_online_account	( )
{
	m_add_new_online_user_bind[0].buffer_type	= MYSQL_TYPE_LONG;
	m_add_new_online_user_bind[0].buffer		= &m_account_id;
	m_add_new_online_user_bind[0].buffer_length	= sizeof( m_account_id );

	m_add_new_online_user_bind[1].buffer_type	= MYSQL_TYPE_VARCHAR;
	m_add_new_online_user_bind[1].buffer		= const_cast< pstr >( m_ip_address.c_str() );
	m_add_new_online_user_bind[1].buffer_length	= m_ip_address.length();

	m_add_new_online_user_bind[2].buffer_type	= MYSQL_TYPE_LONG;
	m_add_new_online_user_bind[2].buffer		= &m_squad_leader_id;
	m_add_new_online_user_bind[2].buffer_length	= sizeof( m_squad_leader_id );

	boost::posix_time::ptime const& now			= boost::posix_time::microsec_clock::local_time();
	
	MYSQL_TIME							current_time;
	current_time.time_type				= MYSQL_TIMESTAMP_DATETIME;
	current_time.year					= now.date().year();
	current_time.month					= now.date().month();
	current_time.day					= now.date().day();
	current_time.hour					= now.time_of_day().hours();
	current_time.minute					= now.time_of_day().minutes();
	current_time.second					= now.time_of_day().seconds();
	current_time.second_part			= u32( now.time_of_day().total_milliseconds() - now.time_of_day().total_seconds() * 1000 );
	current_time.neg					= false;
	
	m_add_new_online_user_bind[3].buffer_type	= MYSQL_TYPE_DATETIME;
	m_add_new_online_user_bind[3].buffer		= &current_time;
	m_add_new_online_user_bind[3].buffer_length	= sizeof( current_time );

	mysql_stmt_bind_param				( m_add_new_online_user, m_add_new_online_user_bind );
	mysql_stmt_execute					( m_add_new_online_user );

	u32 const session_id				= (u32)mysql_insert_id( &m_connection );
	return								session_id;
}

u32 query_processor::set_account_id		( u32 account_id, std::string const& ip_address )
{
	m_account_id						= account_id;
	m_ip_address						= ip_address; 
	return add_new_online_account		( );
}

void query_processor::set_last_activity_time	( )
{
	boost::posix_time::ptime const& now	= boost::posix_time::microsec_clock::local_time();

	MYSQL_TIME						current_time;
	current_time.time_type			= MYSQL_TIMESTAMP_DATETIME;
	current_time.year				= now.date().year();
	current_time.month				= now.date().month();
	current_time.day				= now.date().day();
	current_time.hour				= now.time_of_day().hours();
	current_time.minute				= now.time_of_day().minutes();
	current_time.second				= now.time_of_day().seconds();
	current_time.second_part		= u32( now.time_of_day().total_milliseconds() - now.time_of_day().total_seconds() * 1000 );
	current_time.neg				= false;
	
	m_set_last_activity_time_bind[0].buffer_type	= MYSQL_TYPE_DATETIME;
	m_set_last_activity_time_bind[0].buffer			= &current_time;
	m_set_last_activity_time_bind[0].buffer_length	= sizeof( current_time );

	m_set_last_activity_time_bind[1].buffer_type	= MYSQL_TYPE_LONG;
	m_set_last_activity_time_bind[1].buffer			= &m_session_id;
	m_set_last_activity_time_bind[1].buffer_length	= sizeof( m_session_id );

	mysql_stmt_bind_param			( m_set_last_activity_time, m_set_last_activity_time_bind );
	mysql_stmt_execute				( m_set_last_activity_time );
}

void query_processor::set_session_id( u32 session_id )
{
	m_session_id					= session_id;
	set_last_activity_time			( );
}

u64 query_processor::disconnect_inactive( )
{
	boost::posix_time::ptime const& collection_time	= boost::posix_time::microsec_clock::local_time() - m_no_ping_interval;

	MYSQL_TIME						current_time;
	current_time.time_type			= MYSQL_TIMESTAMP_DATETIME;
	current_time.year				= collection_time.date().year();
	current_time.month				= collection_time.date().month();
	current_time.day				= collection_time.date().day();
	current_time.hour				= collection_time.time_of_day().hours();
	current_time.minute				= collection_time.time_of_day().minutes();
	current_time.second				= collection_time.time_of_day().seconds();
	current_time.second_part		= u32( collection_time.time_of_day().total_milliseconds() - collection_time.time_of_day().total_seconds() * 1000 );
	current_time.neg				= false;
	
	m_disconnect_inactive_users_bind[0].buffer_type		= MYSQL_TYPE_DATETIME;
	m_disconnect_inactive_users_bind[0].buffer			= &current_time;
	m_disconnect_inactive_users_bind[0].buffer_length	= sizeof( current_time );

	mysql_stmt_bind_param			( m_disconnect_inactive_users, m_disconnect_inactive_users_bind );
	mysql_stmt_execute				( m_disconnect_inactive_users );
	u64 const users_collected		= mysql_affected_rows( &m_connection );
	return							users_collected;
}

u64 query_processor::set_no_ping_interval	( boost::posix_time::seconds const& interval )
{
	m_no_ping_interval				= interval;
	return disconnect_inactive		( );
}

} // namespace xray
