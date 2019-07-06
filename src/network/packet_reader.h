////////////////////////////////////////////////////////////////////////////
//	Created		: 23.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_PACKET_READER_H_INCLUDED
#define XRAY_NETWORK_PACKET_READER_H_INCLUDED

namespace xray {
namespace network {

class packet_reader : private boost::noncopyable {
public:
	inline	explicit	packet_reader	( packet const& packet );
	inline	void		r				( pvoid result, u32 destination_size, u32 size );
	//inline	u64		r_u64			( );
	//inline	s64		r_s64			( );
	//inline	u32		r_u32			( );
	//inline	s32		r_s32			( );
	//inline	u16		r_u16			( );
	//inline	s16		r_s16			( );
	//inline	u8		r_u8			( );
	//inline	s8		r_s8			( );
	//inline	float	r_float			( );
	template < typename T >
	inline	T			r				( );
	template < int Count >
	inline	pstr		r_string		( char (&string)[Count] );
	inline	pstr		r_string		( pstr string, u8 buffer_size );

private:
	packet const&	m_packet;
	pcbyte			m_pointer;
}; // class packet_reader

} // namespace network
} // namespace xray

#include "packet_reader_inline.h"

#endif // #ifndef XRAY_NETWORK_PACKET_READER_H_INCLUDED