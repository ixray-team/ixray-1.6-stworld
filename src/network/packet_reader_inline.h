////////////////////////////////////////////////////////////////////////////
//	Created		: 23.02.2012
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2012
////////////////////////////////////////////////////////////////////////////

#ifndef XRAY_NETWORK_PACKET_READER_INLINE_H_INCLUDED
#define XRAY_NETWORK_PACKET_READER_INLINE_H_INCLUDED

namespace xray {
namespace network {

inline packet_reader::packet_reader	( packet const& packet ) :
	m_packet	( packet ),
	m_pointer	( packet.buffer() )
{
}

inline void packet_reader::r		( pvoid const destination, u32 const destination_size, u32 const size )
{
	ASSERT		( m_pointer >= m_packet.buffer() );
	ASSERT		( m_pointer <= m_packet.buffer() + m_packet.buffer_size() );
	ASSERT		( m_packet.buffer() + m_packet.buffer_size() >= (m_pointer + size) );

	memcpy		( destination, m_pointer, size);
	m_pointer	+= size;
}

//inline u64 packet_reader::r_u64		( )
//{
//	return		r< u64 >( );
//}
//
//inline s64 packet_reader::r_s64		( )
//{
//	return		r< s64 >( );
//}
//
//inline u32 packet_reader::r_u32		( )
//{
//	return		r< u32 >( );
//}
//
//inline s32 packet_reader::r_s32		( )
//{
//	return		r< s32 >( );
//}
//
//inline u16 packet_reader::r_u16		( )
//{
//	return		r< u16 >( );
//}
//
//inline s16 packet_reader::r_s16		( )
//{
//	return		r< s16 >( );
//}
//
//inline u8 packet_reader::r_u8		( )
//{
//	return		r< u8 >( );
//}
//
//inline s8 packet_reader::r_s8		( )
//{
//	return		r< s8 >( );
//}
//
//inline float packet_reader::r_float	( )
//{
//	return		r< float >( );
//}

template < typename T >
inline T packet_reader::r			( )
{
	T			result;
	r			( &result, sizeof( result ), sizeof( result ) );
	return		( result );
}

inline pstr packet_reader::r_string	( pstr string, u8 const buffer_size )
{
	u8 const string_length	= r<u8>( );
	ASSERT		( string_length < 255 );
	r			( string, string_length, string_length );
	string[ string_length ]	= 0;
	return		string;
}

template < int Count >
inline pstr packet_reader::r_string	( char (&string)[Count] )
{
	return		r_string( &string[0], (u8)std::min( sizeof(string), size_t(255) ) );
}

} // namespace network
} // namespace xray

#endif // #ifndef XRAY_NETWORK_PACKET_READER_INLINE_H_INCLUDED