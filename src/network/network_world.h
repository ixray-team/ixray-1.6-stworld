////////////////////////////////////////////////////////////////////////////
//	Created		: 24.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_NETWORK_WORLD_H_INCLUDED
#define XRAY_NETWORK_NETWORK_WORLD_H_INCLUDED

#include "two_way_threads_channel.h"

namespace xray {
namespace network {

class network_world {
public:
					network_world		(
						memory::doug_lea_allocator& responses_allocator,
						memory::doug_lea_allocator& orders_allocator
					);
			void	process_orders		( );
			void	process_responses	( );
			void	owner_initialize	( );
			void	user_initialize		( );
			void	add_order			( network_order* order );
			void	add_response		( network_response* response );

private:
	two_way_threads_channel	m_channel;
}; // class network_world

} // namespace network
} // namespace xray

#endif // #ifndef XRAY_NETWORK_NETWORK_WORLD_H_INCLUDED