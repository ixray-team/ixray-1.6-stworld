////////////////////////////////////////////////////////////////////////////
//	Created		: 06.02.2012
//	Author		: Tetyana Meleshchenko
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef QUERY_PROCESSOR_H_INCLUDED
#define QUERY_PROCESSOR_H_INCLUDED

#include "../login_structures.h"
#include <boost/program_options.hpp>

namespace xray {

class query_processor : private boost::noncopyable
{
public:
			query_processor				( MYSQL& connection, boost::program_options::variables_map const& options );
			~query_processor			( );	

	void	set_sign_up_info			( sign_up_info const& info );
	void	set_next_attempt_time		(
				u32 account_id,
				u32 invalid_attempts_count,
				boost::posix_time::ptime const& next_attempt_time
			);
	u32		set_account_id				( u32 account_id, std::string const& ip_address );
	void	set_session_id				( u32 session_id );
	u64		set_no_ping_interval		( boost::posix_time::seconds const& interval );

private:
	void	add_new_account				( );
	void	update_sign_in_stats		( );
	u32		add_new_online_account		( );
	void	set_last_activity_time		( );
	u64		disconnect_inactive			( );
	
private:
	MYSQL&								m_connection;
	
	MYSQL_STMT*							m_add_new_user;
	MYSQL_STMT*							m_update_sign_in_stats;
	MYSQL_STMT*							m_add_new_online_user;
	MYSQL_STMT*							m_set_last_activity_time;
	MYSQL_STMT*							m_disconnect_inactive_users;

	MYSQL_BIND							m_add_new_user_bind[4];
	MYSQL_BIND							m_update_sign_in_stats_bind[3];
	MYSQL_BIND							m_add_new_online_user_bind[4];
	MYSQL_BIND							m_set_last_activity_time_bind[2];
	MYSQL_BIND							m_disconnect_inactive_users_bind[1];

	sign_up_info						m_sign_up_info;
	std::string							m_ip_address;
	boost::posix_time::ptime			m_next_attempt_time;
	boost::posix_time::seconds			m_no_ping_interval;
	u32									m_account_id;
	u32									m_invalid_attempts_count;
	u32									m_squad_leader_id;
	u32									m_session_id;
}; // class query_processor

} // namespace xray

#endif // #ifndef QUERY_PROCESSOR_H_INCLUDED
