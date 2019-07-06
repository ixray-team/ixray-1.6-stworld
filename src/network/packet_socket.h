////////////////////////////////////////////////////////////////////////////
//	Created		: 23.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_NETWORK_PACKET_SOCKET_H_INCLUDED
#define ENGINE_NETWORK_PACKET_SOCKET_H_INCLUDED

#include "../login_server/memory_allocator.h"
#include "packet.h"
#include "client_error_codes.h"

namespace xray {
namespace network {

class packet;
enum client_error_codes_enum;

template < typename SocketType >
class packet_socket : private boost::noncopyable {
public:
	inline				packet_socket			( SocketType& socket ) : m_socket( socket ) { }
	inline	void		send					( packet const& packet );
	inline	SocketType&	next_layer				( ) const { return m_socket; }
	inline	SocketType&	lowest_layer			( ) const { return m_socket; }
	inline	void		start_receiving			( );
	inline	void		stop_receiving			( );

public:
	typedef boost::function< void ( packet const& packet ) >	on_packet_received_functor_type;
	typedef boost::function<
		void (
			client_error_codes_enum,
			boost::system::error_code
		) >			on_error_functor_type;

public:
	inline	void		set_on_packet_received	( on_packet_received_functor_type const& functor )	{ m_on_packet_received = functor; }
	inline	void		set_on_error			( on_error_functor_type const& functor )			{ m_on_error = functor; }

private:
	inline	void		on_packet_received		( packet const* packet, boost::system::error_code const& error_code, size_t bytes_transferred );
	template < typename BufferSizeType >
	inline	void		on_packet_size_received	( boost::system::error_code const& error_code, size_t bytes_transferred );
	inline	void		on_packet_has_been_sent	(
							packet const* packet_being_sent,
							boost::system::error_code const& error_code,
							size_t bytes_transferred
						);
	inline	packet*		new_packet				( );
	inline	void		delete_packet			( packet const*& packet );

private:
	on_packet_received_functor_type	m_on_packet_received;
	on_error_functor_type			m_on_error;
	handler_allocator				m_allocator;
	SocketType&						m_socket;
	u32								m_header_buffer;
}; // class packet_socket

} // namespace network
} // namespace xray

#include "packet_socket_inline.h"

#endif // #ifndef ENGINE_NETWORK_PACKET_SOCKET_H_INCLUDED