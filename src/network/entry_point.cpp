////////////////////////////////////////////////////////////////////////////
//	Created		: 21.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "client.h"
#include "server.h"
#include "packet.h"
#include "client_session.h"
#include <boost/thread.hpp>
#include "packet.h"
#include "packet_reader.h"
#include "network_world.h"
#include <locale.h>

namespace xray {
namespace threading {

u32 current_thread_id	( )
{
#ifdef WIN32
	return			GetCurrentThreadId( );
#else // #ifdef WIN32
	thread_t		result;
	thread_get_id	( &result );
	return			result;
#endif // #ifdef WIN32
}

} // namespace threading
} // namespace xray

xray::network::network_world* g_world = 0;

static bool do_exit	= false;

static void server_on_packet_received	( xray::network::server& server, xray::network::client_session& client_session, xray::network::packet const& packet )
{
	xray::network::packet_reader reader( packet );
	char string[256];
	reader.r_string					( string );

	xray::network::packet answer_packet;
	answer_packet.append			( string );
	client_session.send				( answer_packet );
}

static xray::network::action_id_enum server_on_client_connected	( xray::network::server& server, xray::network::client_session& client_session )
{
	client_session.set_on_packet_received	( boost::bind( &server_on_packet_received, boost::ref(server), boost::ref(client_session), _1 ) );
	return							xray::network::permit_connection;
}

static void client_on_connected			( xray::network::client& client )
{
}

class network_send_packet_order : public xray::network::network_order {
public:
	inline			network_send_packet_order	(
			xray::network::client& client,
			xray::network::packet const& packet
		) :
		m_client	( client ),
		m_packet	( packet )
	{
	}

	virtual			~network_send_packet_order	( )
	{
		delete		&m_packet;
	}

	virtual	void	execute						( )
	{
		m_client.send				( m_packet );
	}

private:
	xray::network::client&			m_client;
	xray::network::packet const&	m_packet;
}; // network_send_packet_order

struct client_wrapper {
	typedef boost::function< void (client_wrapper& client, xray::network::packet_reader& ) >	on_packet_received_type;

	inline		client_wrapper		(
			xray::network::client& client,
			xray::memory::doug_lea_allocator& orders_allocator,
			xray::memory::doug_lea_allocator& responses_allocator
		) :
		m_client					( client ),
		m_orders_allocator			( orders_allocator ),
		m_responses_allocator		( responses_allocator )
	{
	}

	inline void	send				( xray::network::packet const* packet )
	{
		g_world->add_order			( new(m_orders_allocator.malloc_impl(sizeof(network_send_packet_order)) ) network_send_packet_order( m_client, *packet ) );
	}

	void on_packet_received_impl	( xray::network::packet const& packet )
	{
		if ( m_on_packet_received ) {
			xray::network::packet_reader packet_reader( packet );
			m_on_packet_received	( *this, packet_reader );
		}
	}

	void on_packet_received			( xray::network::packet const& packet );

	void set_on_packet_received		( on_packet_received_type const& on_packet_received )
	{
		if ( !m_on_packet_received && on_packet_received ) {
			m_client.set_on_packet_received	( boost::bind( &client_wrapper::on_packet_received, this, _1 ) );
		}
		else if ( m_on_packet_received && !on_packet_received ) {
			m_client.set_on_packet_received	( xray::network::client::on_packet_received_functor_type() );
		}

		m_on_packet_received		= on_packet_received;
	}
	
	on_packet_received_type				m_on_packet_received;
	xray::network::client&				m_client;
	xray::memory::doug_lea_allocator&	m_orders_allocator;
	xray::memory::doug_lea_allocator&	m_responses_allocator;
}; // struct client_wrapper

class network_receive_packet_response : public xray::network::network_response {
public:
	inline			network_receive_packet_response(
			client_wrapper& client,
			xray::network::packet const& packet
		) :
		m_client	( client ),
		m_packet	( packet )
	{
	}

	virtual			~network_receive_packet_response( )
	{
		m_packet.~packet	( );
		m_client.m_responses_allocator.free_impl( const_cast<xray::network::packet*>(&m_packet) );
	}

	virtual	void	execute					( )
	{
		m_client.on_packet_received_impl( m_packet );
	}

private:
	client_wrapper&					m_client;
	xray::network::packet const&	m_packet;
}; // network_receive_packet_response

void client_wrapper::on_packet_received	( xray::network::packet const& packet )
{
	if ( !m_on_packet_received )
		return;

	xray::network::packet* cloned_packet	= new(m_responses_allocator.malloc_impl(sizeof(xray::network::packet)) ) xray::network::packet();
	cloned_packet->clone			( packet );
	g_world->add_response			( new(m_responses_allocator.malloc_impl(sizeof(network_receive_packet_response)) ) network_receive_packet_response( *this, *cloned_packet ) );
}

static bool s_initialized			= false;
static xray::network::client* s_client	= 0;

void network_thread_entry_point			( xray::memory::doug_lea_allocator& responses )
{
	responses.user_current_thread_id( );

	g_world->owner_initialize		( );

	u32 const port					= 2510;

	boost::asio::io_service server_service;
	xray::network::server server( server_service );
	server.set_on_connected			( boost::bind( &server_on_client_connected, boost::ref(server), _1 ) );

	boost::asio::io_service client_service;
	xray::network::client client( client_service );
	s_client						= &client;

	client.set_on_connected			( boost::bind( &client_on_connected, boost::ref(client) ) );

	server.accept_connections		( port, 1 );
	client.connect					( "localhost", port );

	while ( !do_exit ) {
		g_world->process_orders		( );
		client_service.poll			( );
		server_service.poll			( );

		if ( !s_initialized && client.is_connected() )
			s_initialized			= true;

		boost::this_thread::sleep	( boost::posix_time::milliseconds(10) );
	}
}

static void on_packet_received			( client_wrapper& client, xray::network::packet_reader& packet )
{
	char temp[256];
	packet.r_string					( temp );
	xray::network::packet* const answer = new xray::network::packet( );
	answer->append					( "regular packet" );
	client.send						( answer );
}

int main								( int const argc, char const* const argv[] )
{
	setlocale						( LC_ALL, "" );

	xray::memory::doug_lea_allocator responses, orders;
	responses.initialize			( malloc(64*1024), 64*1024, "network responses" );
	orders.initialize				( malloc(64*1024), 64*1024, "network orders" );

	g_world							= new xray::network::network_world( responses, orders );
	g_world->user_initialize		( );

	boost::thread thread( boost::bind(&network_thread_entry_point, boost::ref(responses) ) );

	while ( !s_initialized )
		boost::this_thread::sleep	( boost::posix_time::milliseconds(10) );

	ASSERT							( s_client );
	client_wrapper client( *s_client, orders, responses );
	client.set_on_packet_received	( &on_packet_received );

	while ( !do_exit ) {
		g_world->process_responses	( );

		if ( static bool first_time = true ) {
			first_time				= false;
			xray::network::packet* const packet = new xray::network::packet( );
			packet->append			( "Hello, world!" );
			client.send				( packet );
		}
		boost::this_thread::sleep	( boost::posix_time::milliseconds(10) );
	}

	thread.join						( );

	delete							g_world;
}