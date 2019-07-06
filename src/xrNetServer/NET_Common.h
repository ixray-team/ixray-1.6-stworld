#if !defined _INCDEF_NETCOMMON_H_
#define _INCDEF_NETCOMMON_H_
#pragma once
//==============================================================================

struct GameDescriptionData
{
	string128	map_name;
	string128	map_version;
	string512	download_url;
};

///////////////
#define	GAMESPY_QR2_BASEPORT				5445
#define GAMESPY_BROWSER_MAX_UPDATES			20

#define START_PORT							0
#define END_PORT							65535
#define START_PORT_LAN						GAMESPY_QR2_BASEPORT
#define START_PORT_LAN_SV					START_PORT_LAN + 1
#define START_PORT_LAN_CL					START_PORT_LAN + 2
#define END_PORT_LAN						START_PORT_LAN + 250//GameSpy only process 500 ports
///////////////

#define NET_MERGE_PACKETS               1
#define NET_TAG_MERGED                  0xE1
#define NET_TAG_NONMERGED               0xE0

#define NET_USE_COMPRESSION             1
#define NET_TAG_COMPRESSED              0xC1
#define NET_TAG_NONCOMPRESSED           0xC0

#define NET_USE_LZO_COMPRESSION         1
#define NET_USE_COMPRESSION_CRC         1

#define NET_LOG_PACKETS                 0
#define NET_LOG_COMPRESSION             0
#define NET_DUMP_COMPRESSION            0


#define NET_GUARANTEEDPACKET_DEFAULT    0
#define NET_GUARANTEEDPACKET_IGNORE     1
#define NET_GUARANTEEDPACKET_SEPARATE   2

extern XRNETSERVER_API int psNET_GuaranteedPacketMode;

struct XRNETSERVER_API ip_address
{
	union{
		struct{
			u8	a1;
			u8	a2;
			u8	a3;
			u8	a4;
		};
		u32		data;
	}m_data;
	void		set		(LPCSTR src_string);
	xr_string	to_string	()	const;

	bool operator == (const ip_address& other) const
	{
		return (m_data.data==other.m_data.data)		|| 
				(	(m_data.a1==other.m_data.a1)	&& 
					(m_data.a2==other.m_data.a2)	&& 
					(m_data.a3==other.m_data.a3)	&& 
					(m_data.a4==0)					);
	}
};


class XRNETSERVER_API MultipacketSender
{
public:
                    MultipacketSender();
    virtual         ~MultipacketSender() {}

    void            SendPacket( const void* packet_data, u32 packet_sz, u32 flags );
    void            FlushSendBuffer( u32 timeout );


protected:

    virtual void    _SendTo_LL( const void* data, u32 size, u32 flags ) =0;


private:

    struct Buffer;


    void            _FlushSendBuffer( u32 timeout, Buffer* buf );

    struct
    Buffer
    {
                    Buffer() : last_flags(0) { buffer.B.count = 0; }
                    
        NET_Packet  buffer;
        u32         last_flags;
    };

    Buffer              _buf;
    Buffer              _gbuf;
    xrCriticalSection   _buf_cs;
};


//==============================================================================

class XRNETSERVER_API MultipacketReciever
{
public:

    virtual         ~MultipacketReciever() {}

    void            RecievePacket( const void* packet_data, u32 packet_sz, u32 param=0 );


protected:

    virtual void    _Recieve( const void* data, u32 data_size, u32 param ) =0;
};


//==============================================================================
#endif // _INCDEF_NETCOMMON_H_

