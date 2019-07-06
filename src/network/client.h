////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_CLIENT_H_INCLUDED
#define XRAY_NETWORK_CLIENT_H_INCLUDED

#include "packet_socket.h"

namespace xray {
namespace network {

class packet;
enum client_error_codes_enum;

class client : private boost::noncopyable {
public:
	explicit		client						( boost::asio::io_service& io_service );
					~client						( );
			void	connect						( pcstr host, u16 port );
			void	disconnect					( );
			void	send						( packet const& packet );
	inline	bool	is_connected				( ) const { return m_connection_state == connection_has_been_established; }
	inline	bool	has_connection_established	( ) const { return m_connection_state >= connection_has_been_established; }

public:
	typedef boost::function< void ( ) >							on_connected_functor_type;
	typedef boost::function< void ( ) >							on_disconnected_functor_type;
	typedef boost::function< void ( packet const& packet ) >	on_packet_received_functor_type;
	typedef boost::function<
		void (
			client_error_codes_enum,
			boost::system::error_code
		) >			on_error_functor_type;

public:
	inline	void	set_on_connected			( on_connected_functor_type const& functor )		{ m_on_connected = functor; }
	inline	void	set_on_disconnected			( on_disconnected_functor_type const& functor )		{ m_on_disconnected = functor; }
	inline	void	set_on_packet_received		( on_packet_received_functor_type const& functor )	{ m_packet_socket.set_on_packet_received( functor ); }
	inline	void	set_on_error				( on_error_functor_type const& functor )			{ m_on_error = functor; }

private:
			void	on_connected				(
						boost::system::error_code const& error_code,
						tcp::resolver::iterator iterator
					);
			void	connect						( tcp::resolver::iterator const& iterator );
			void	on_resolved					(
						tcp::resolver* const resolver,
						boost::system::error_code const& error_code,
						tcp::resolver::iterator iterator
					);
			void	resolve						( pcstr host, u32 port );
			void	close_connection			( );

			void	on_error					( client_error_codes_enum client_error_code, boost::system::error_code error_code );
			void	start_reading				( );

private:
	enum connection_state_enum {
		host_name_is_unresolved,
		host_name_is_being_resolved,
		host_name_has_been_resolved,

		connection_is_being_established,
		connection_has_been_established,

		//client_is_being_verified,
		//client_has_been_verified,
	}; // enum connection_state_enum

private:
	// ordering of the next two members is important here
	tcp::socket						m_socket;
	packet_socket< tcp::socket >	m_packet_socket;

	tcp::resolver::iterator			m_host;
	on_connected_functor_type		m_on_connected;
	on_disconnected_functor_type	m_on_disconnected;
	on_packet_received_functor_type	m_on_packet_received;
	on_error_functor_type			m_on_error;
	handler_allocator				m_allocator;
	boost::asio::io_service&		m_io_service;
	packet*							m_first_packet;
	connection_state_enum			m_connection_state;
}; // class client

} // namespace network
} // namespace xray

#endif // #ifndef XRAY_NETWORK_CLIENT_H_INCLUDED