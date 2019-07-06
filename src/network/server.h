////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_SERVER_H_INCLUDED
#define XRAY_NETWORK_SERVER_H_INCLUDED

#include "../login_server/memory_allocator.h"

namespace xray {
namespace network {

class packet;
class client_session;

enum action_id_enum {
	permit_connection,
	forbid_connection,
}; // enum action_id_enum

class server : private boost::noncopyable {
public:
	typedef boost::function< action_id_enum ( client_session& ) >	on_connected_functor_type;
	typedef boost::function< void ( client_session& ) >				on_disconnected_functor_type;

public:
	explicit		server					( boost::asio::io_service& io_service );
					~server					( );
			void	accept_connections		( u16 port, u32 max_connections_count );
			void	send_broadcast			( packet const& packet );
			void	set_on_connected		( on_connected_functor_type const& functor );
			void	set_on_disconnected		( on_disconnected_functor_type const& functor );

private:
			void	delete_client_session	( client_session* client_session );
			void	process_client			(
						client_session* new_client,
						boost::system::error_code const& error
					);
			void	start_client_acception	( );

private:
	on_connected_functor_type		m_on_connected;
	on_disconnected_functor_type	m_on_disconnected;
	tcp::acceptor					m_client_acceptor;
	handler_allocator				m_socket_allocator;
	boost::asio::io_service&		m_io_service;
	client_session*					m_first_client;
	u32								m_max_connections_count;
	u32								m_connections_count;
}; // class server

} // namespace network
} // namespace xray

#endif // #ifndef XRAY_NETWORK_SERVER_H_INCLUDED