////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_CLIENT_SESSION_H_INCLUDED
#define XRAY_NETWORK_CLIENT_SESSION_H_INCLUDED

#include "packet_socket.h"

namespace xray {
namespace network {

class server;

class client_session {
public:
	typedef packet_socket< tcp::socket >	packet_socket_type;
public:
							client_session	(
								boost::asio::io_service& io_service,
								server& server
							);
							~client_session	( );
			void			start			( );
	inline	tcp::socket&	socket			( ) { return m_socket; }
	inline	void	set_on_packet_received	( packet_socket_type::on_packet_received_functor_type const& functor )	{ m_packet_socket.set_on_packet_received( functor ); }
	inline	void			send			( packet const& packet ) { m_packet_socket.send(packet); }

private:
	// ordering of the next two members is important here
	tcp::socket				m_socket;
	packet_socket_type		m_packet_socket;

public:
	client_session*			next;
}; // class client_session

} // namespace network
} // namespace xray

#endif // #ifndef XRAY_NETWORK_CLIENT_SESSION_H_INCLUDED