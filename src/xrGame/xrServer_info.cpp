#include "stdafx.h"
#include "xrServer_info.h"
#include "level.h"
#include "xrserver.h"

#define SERVER_LOGO_FN	"server_logo.jpg"
#define SERVER_RULES_FN "server_rules.txt"

server_info_uploader::server_info_uploader(file_transfer::server_site* file_transfers) :
	m_state(eUploadNotActive),
	m_logo_data(NULL),
	m_logo_size(0),
	m_rules_data(NULL),
	m_rules_size(0),
	m_file_transfers(file_transfers)
{
	R_ASSERT(Level().Server && Level().Server->GetServerClient());
	m_from_client = Level().Server->GetServerClient()->ID;
}

server_info_uploader::~server_info_uploader()
{
	R_ASSERT(m_file_transfers != NULL);
	if (is_active())
		terminate_upload();
}


void server_info_uploader::terminate_upload()
{
	R_ASSERT(is_active());
	m_file_transfers->stop_transfer_file(std::make_pair(m_to_client, m_from_client));
	m_state = eUploadNotActive;
	execute_complete_cb();
}

void server_info_uploader::start_upload_info	(IReader const * svlogo, 
												 IReader const * svrules,
												 ClientID const & toclient,
												 svinfo_upload_complete_cb const & complete_cb)
{
	using namespace file_transfer;
	sending_state_callback_t	sndcb;
	sndcb.bind(this, &server_info_uploader::upload_server_info_callback);
	
	buffer_vector<mutable_buffer_t>	tmp_bufvec(
		_alloca(sizeof(mutable_buffer_t) * 2),
		2
	);
	
	tmp_bufvec.push_back(
		std::make_pair(
			static_cast<u8*>(svlogo->pointer()),
			svlogo->length()
		)
	);
	
	tmp_bufvec.push_back(
		std::make_pair(
			static_cast<u8*>(svrules->pointer()),
			svrules->length()
		)
	);
	
	m_to_client = toclient;

	m_file_transfers->start_transfer_file(
		tmp_bufvec,
		m_to_client,
		m_from_client,
		sndcb,
		0
	);
	m_state			= eUploadingInfo;
	m_complete_cb	= complete_cb;
}

void server_info_uploader::execute_complete_cb()
{
	R_ASSERT(m_complete_cb);
	m_complete_cb		(m_to_client);
	m_complete_cb.clear	();
}

void __stdcall server_info_uploader::upload_server_info_callback(
	file_transfer::sending_status_t status,
	u32 uploaded, u32 total)
{
	switch (status)
	{
	case file_transfer::sending_data:
		{
#ifdef DEBUG
			Msg("* uploaded %d from %d bytes of server logo to client [%d]", uploaded, total, m_to_client.value());
#endif
			return;
		}break;
	case file_transfer::sending_aborted_by_user:
		{
			FATAL("* upload server logo terminated by user ");
		}break;
	case file_transfer::sending_rejected_by_peer:
		{
			Msg("* upload server logo terminated by peer [%d]", m_to_client.value());
		}break;
	case file_transfer::sending_complete:
		{
			Msg("* upload server info to client [%d] complete !", m_to_client.value());
		}break;
	};
	m_state = eUploadNotActive;
	execute_complete_cb();
}