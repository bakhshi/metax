/**
 * @file src/metax_web_api/web_api_adapter.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::web_api_adapter.
 *
 * Copyright (c) 2015-2019 Leviathan CJSC
 *  Email: info@leviathan.am
 *  134/1 Tsarav Aghbyur St.,
 *  Yerevan, 0052, Armenia
 *  Tel:  +1-408-625-7509
 *        +49-893-8157-1771
 *        +374-60-464700
 *        +374-10-248411
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

// Headers from this project
#include "web_api_adapter.hpp"
#include "websocket_request_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include "Poco/JSON/Parser.h"
#include "Poco/File.h"
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/Net/HTTPServerRequest.h>

// Headers from standard libraries
#include <iostream>

leviathan::metax_web_api::web_api_adapter*
leviathan::metax_web_api::web_api_adapter::s_instance = nullptr;

leviathan::metax_web_api::web_api_adapter*
leviathan::metax_web_api::web_api_adapter::
get_instance()
{
        if (nullptr == s_instance) {
                s_instance = new web_api_adapter();
        }
        return s_instance;
}

void leviathan::metax_web_api::web_api_adapter::
delete_instance()
{
        // web_api_adapter class should be deleted by Poco Thread Pool when its
        // runTask is finished. Here we just assign nullptr to instance pointer
        s_instance = nullptr;
}

void leviathan::metax_web_api::web_api_adapter::
set_configuration(const std::string& cfg_path)
{
        try {
                Poco::AutoPtr<Poco::Util::XMLConfiguration> c(
                                new Poco::Util::XMLConfiguration(cfg_path));
                m_enable_non_localhost_save =
                        c->getBool("enable_non_localhost_save", false);
                m_enable_non_localhost_get =
                        c->getBool("enable_non_localhost_get", true);
        } catch (Poco::RuntimeException& e) {
                METAX_WARNING("Failed parsing configuration: "
                                + e.displayText());
        }
}

bool leviathan::metax_web_api::web_api_adapter::
is_save_allowed(const Poco::Net::HTTPServerRequest& req)
{
        return req.clientAddress().host().isLoopback() ||
                m_enable_non_localhost_save;
}

bool leviathan::metax_web_api::web_api_adapter::
is_get_allowed(const Poco::Net::HTTPServerRequest& req)
{
        return req.clientAddress().host().isLoopback() ||
                m_enable_non_localhost_get;
}

leviathan::metax::ID32
leviathan::metax_web_api::web_api_adapter::
generate_request_id()
{
        if (UINT32_MAX == m_counter) {
                m_counter = 0;
        }
        return m_counter++;
}

void leviathan::metax_web_api::web_api_adapter::
add_websocket(websocket_request_handler* ws)
{
        Poco::ScopedLock<Poco::Mutex> gm(m_wsockets_mutex);
        for (auto& m : m_last_messages) {
                bool b = ws->send(m);
                if (! b) {
                        return;
                }
        }
        m_wsockets.insert(ws);
}

void leviathan::metax_web_api::web_api_adapter::
remove_websocket(websocket_request_handler* ws)
{
        Poco::ScopedLock<Poco::Mutex> gm(m_wsockets_mutex);
        METAX_INFO("Websocket connection closed - " + ws->peer_address);
        ws->close();
        m_wsockets.erase(ws);
}

leviathan::metax::ID32 leviathan::metax_web_api::web_api_adapter::
generate_get_request_info(std::promise<ims_response>* pr,
                                        unsigned long int client_id)
{
        metax::ID32 id = generate_request_id();
        Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
        Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
        m_get_requests.insert(std::make_pair(id,
                                request_info(id, pr, "", "",
                                        Poco::UUID::null(), metax::get,
                                        client_id)));
        m_client_to_request.insert(std::make_pair(client_id, id));
        return id;
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_request(std::promise<ims_response>* pr, const std::string& r,
                                const std::pair<int64_t, int64_t>& range,
                                bool cache, unsigned long int client_id)
{
        METAX_TRACE(__FUNCTION__);
        metax::ID32 id = 0;
        try {
                id = generate_get_request_info(pr, client_id);
                Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                metax::ims_kernel_package& out = *metax_tx;
                out.request_id = id;
                out.uuid.parse(r);
                out.cmd = metax::get;
                out.get_range = range;
                out.set_payload(r);
                out.cache = cache;
                metax_tx.commit();
                METAX_INFO("Request ID: " + UTILS::to_string(id));
                Poco::Timestamp now;
                METAX_INFO("!test_log!get_request!uuid!" + r + "!time!" +
                              UTILS::to_string(now.epochMicroseconds()) + '!');
        } catch (...) {
                clean_get_request(id, client_id);
                throw;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_copy_request(std::promise<ims_response>* pr, const std::string& r,
                Poco::Int64 expire_date, unsigned long int client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::UUID u(r);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                                "", "", u, metax::copy)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.uuid = u;
        out.cmd = metax::copy;
        out.expire_date = expire_date;
        metax_tx.commit();
        METAX_INFO("Request ID: " + UTILS::to_string(id));
        Poco::Timestamp now;
        METAX_INFO("!test_log!get_request!uuid!" + r + "!time!" +
                        UTILS::to_string(now.epochMicroseconds()) + '!');
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_post_request(std::promise<web_api_adapter::ims_response>* pr,
                  const std::string& s, const std::string& c, URI b,
                  const Poco::UUID& u, bool enc,
                  metax::ims_kernel_package::save_option l, Poco::Int64 e)
{
        METAX_TRACE(__FUNCTION__);
        const metax::ID32 id = generate_request_id();
        try {
                //Saves temporary files path what must be deleted then
                const std::string& p = (metax_web_api::save_data == b) ? s : "";
                {
                        Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                        m_save_requests.insert(std::make_pair(id,
                                                request_info(id, pr, p)));
                }
                Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                metax::ims_kernel_package& out = *metax_tx;
                out.request_id = id;
                out.cmd = u.isNull() ? metax::save : metax::update;
                out.uuid = u;
                out.local = l;
                out.enc = enc;
                out.expire_date = e;
                if (metax_web_api::save_path == b
                                || metax_web_api::save_data == b) {
                        out.file_path = s;
                } else {
                        out.set_payload(s);
                };
                out.content_type = c;
                //TODO: Handle set_payload when got data in HMI instead of path
                metax_tx.commit();
        } catch (...) {
                clean_save_request(id);
                throw;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_save_stream_request(std::promise<web_api_adapter::ims_response>* pr,
                                  const std::string& s, const std::string& c,
                                  const Poco::UUID& uuid,
                                  bool enc, unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        const metax::ID32 id = generate_request_id();
        try {
                {
                        Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                        Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
                        m_save_requests.insert(std::make_pair(id,
                                                request_info(id, pr, s)));
                        m_client_to_request.insert(std::make_pair(client_id, id));
                }
                Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                metax::ims_kernel_package& out = *metax_tx;
                out.request_id = id;
                out.cmd = metax::save_stream;
                out.enc = enc;
                out.uuid = uuid;
                out.content_type = c;
                metax_tx.commit();
        } catch (...) {
                clean_save_request(id);
                throw;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_livestream_content(const char* buf, int size, unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::livestream_content;
        out.resize((uint32_t)size);
        out.set_payload(buf);
        std::map<long unsigned int, metax::ID32>::const_iterator ci;
        {
                Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
                ci = m_client_to_request.find(client_id);
                poco_assert(m_client_to_request.end() != ci);
        }
        out.request_id = ci->second;
        out.file_path = "";
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
cancel_livestream(unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        std::map<long unsigned int, metax::ID32>::const_iterator ci;
        {
                Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
                ci = m_client_to_request.find(client_id);
                poco_assert(m_client_to_request.end() != ci);
        }
        if (m_save_requests.end() != m_save_requests.find(ci->second)) {
                Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                metax::ims_kernel_package& out = *metax_tx;
                out.cmd = metax::livestream_cancel;
                out.request_id = ci->second;
                metax_tx.commit();
                clean_save_request(ci->second, client_id);
        } else {
                m_client_to_request.erase(client_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
cancel_livestream_get(unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        // Check whether the stream is already cleaned up by router.
        std::map<long unsigned int, metax::ID32>::const_iterator ci;
        {
                Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
                ci = m_client_to_request.find(client_id);
                poco_assert(m_client_to_request.end() != ci);
        }
        if (m_get_requests.end() == m_get_requests.find(ci->second)) {
                return;
        }
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::cancel_livestream_get;
        out.request_id = ci->second;
        metax_tx.commit();
        clean_get_request(ci->second, client_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_share_request(std::promise<ims_response>* pr, const std::string& s,
                const Poco::UUID& u, const bool need_peer_key)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing share request for " + u.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                const std::string& uork = need_peer_key ? "" : s;
                const std::string& uk = need_peer_key ? s : "";
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                m_save_requests.insert(std::make_pair(id, request_info(id, pr,
                                uork, uk, u, metax::share)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        if (need_peer_key) {
                out.cmd = metax::get_friend;
        } else {
                out.cmd = metax::share;
                out.uuid = u;
        }
        out.set_payload(s);
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_accept_share_request(std::promise<ims_response>* pr,
                const Poco::UUID& u, const std::string& k,
                const std::string& i)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing accept share request for "
                        + u.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                m_save_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", u, metax::accept_share)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::accept_share;
        out.uuid = u;
        out.aes_key = k;
        out.aes_iv = i;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_register_on_update_request(std::promise<ims_response>* pr, const Poco::UUID& u)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing register listener request for "
                        + u.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", u, metax::on_update_register)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::on_update_register;
        out.uuid = u;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_unregister_on_update_request(std::promise<ims_response>* pr, const Poco::UUID& u)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing unregister listener request for "
                        + u.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", u, metax::on_update_unregister)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::on_update_unregister;
        out.uuid = u;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_delete_request(std::promise<ims_response>* pr, const Poco::UUID& u,
                bool keep_chunks)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing delete request for "
                        + u.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", u, metax::del)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::del;
        out.uuid = u;
        out.keep_chunks = keep_chunks;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_dump_user_info(std::promise<ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing dump user info request.");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                                "", "", Poco::UUID::null(),
                                                metax::dump_user_info)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::dump_user_info;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_recording_start_request(std::promise<ims_response>* pr,
                                                const Poco::UUID& lu)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing start recording request for "
                        + lu.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", lu, metax::recording_start)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::recording_start;
        out.uuid = lu;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_recording_stop_request(std::promise<ims_response>* pr,
                                const Poco::UUID& lu, const Poco::UUID& ru)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Processing delete request for "
                        + lu.toString() + " uuid");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr,
                                "", "", lu, metax::recording_stop)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::recording_stop;
        out.uuid = lu;
        out.rec_uuid = ru;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_move_cache_to_storage_request(
                std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Received move cache to storage request");
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                m_get_requests.insert(std::make_pair(id, request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::move_cache_to_storage;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_trusted_peer_request(std::promise<web_api_adapter::ims_response>* pr,
                const std::string& k)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr, k)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::add_peer;
        out.set_payload(k);
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_remove_trusted_peer_request(std::promise<web_api_adapter::ims_response>* pr,
                const std::string& k)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr, k)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::remove_peer;
        out.set_payload(k);
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_friend_request(std::promise<web_api_adapter::ims_response>* pr,
                const std::string& u, const std::string& k)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr, u)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::add_friend;
        out.user_id.set_payload(k);
        out.set_payload(u);
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_friend_list_request(
                std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_friend_list;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_user_public_key_request(
                std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_public_key;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_user_keys_request(
                std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_user_keys;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_online_peers(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_online_peers;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_metax_info(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_metax_info;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_set_metax_info(std::promise<web_api_adapter::ims_response>* pr,
                                                        const std::string& u)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.set_payload(u);
        out.cmd = metax::set_metax_info;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_reconnect_to_peers(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                        "", "text/plain");
        pr->set_value(ir);
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::reconnect_to_peers;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_start_pairing(std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& timeout)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.set_payload(timeout);
        out.cmd = metax::start_pairing;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_cancel_pairing(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                        "", "text/plain");
        pr->set_value(ir);
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::cancel_pairing;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_pairing_peers(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_pairing_peers;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_request_keys(std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& ip, const std::string& code)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        std::string payload = ip + ',' + code;
        out.set_payload(payload);
        out.request_id = id;
        out.cmd = metax::request_keys;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_regenerate_user_keys(std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr, "", "",
                                                Poco::UUID::null(),
                                                metax::regenerate_user_keys)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::regenerate_user_keys;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_trusted_peer_list_request(
                std::promise<web_api_adapter::ims_response>* pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr);
        const metax::ID32 id = generate_request_id();
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                m_config_requests.insert(std::make_pair(id,
                                        request_info(id, pr)));
        }
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::get_peer_list;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
clean_streaming_request(unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
        Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
        auto ci = m_client_to_request.find(client_id);
        poco_assert(m_client_to_request.end() != ci);
        std::map<metax::ID32, request_info>::iterator ri =
                                                m_get_requests.find(ci->second);
        poco_assert(m_get_requests.end() != ri);
        request_info& req = ri->second;
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = req.id;
        out.cmd = metax::clean_stream_request;
        metax_tx.commit();
        clean_get_request(req.id, client_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_send_to_peer_request(std::promise<ims_response>* pr,
          const std::string& user, const bool need_peer_key,
          const std::string& data)
{
        METAX_TRACE(__FUNCTION__);
        const metax::ID32 id = generate_request_id();
        {
                const std::string& uord = need_peer_key ? data : "";
                const std::string& uk = need_peer_key ? user : "";
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                m_save_requests.insert(std::make_pair(id, request_info(id, pr,
                        uord, uk, Poco::UUID::null(), metax::send_to_peer)));
        }
        if (need_peer_key) {
                Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                metax::ims_kernel_package& out = *metax_tx;
                out.request_id = id;
                out.cmd = metax::get_friend;
                out.set_payload(user);
                metax_tx.commit();
        } else {
                post_send_to_peer_request(id, user, data);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
post_send_to_peer_request(metax::ID32 id, const std::string& user,
                const std::string& data)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.request_id = id;
        out.cmd = metax::send_to_peer;
        out.user_id.set_payload(user);
        out.set_payload(data);
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_friend_confirm()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_friend_found == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                poco_assert(0 != in.message.get());
                if (metax::share == i->second.command) {
                        Poco::ScopedLock<Poco::Mutex> gm(m_metax_tx_mutex);
                        metax::ims_kernel_package& out = *metax_tx;
                        out.cmd = metax::share;
                        out.uuid = i->second.uuid;
                        i->second.info = in.message.get();
                        out.set_payload(in);
                        out.request_id = in.request_id;
                        metax_tx.commit();
                } else {
                        poco_assert(metax::send_to_peer == i->second.command);
                        post_send_to_peer_request(in.request_id,
                                        in.message.get(), i->second.info);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_friend_not_found()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_friend_not_found == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_ERROR("Could not find friend");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = R"({"error":"Peer not found for the request with )";
                s += i->second.username.empty() ? "public key" : i->second.username;
                s += R"("})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_trusted_peer_confirm()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::add_peer_confirm == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Trusted peer added successfully");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_remove_trusted_peer_confirm()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::remove_peer_confirm == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Trusted peer removeed successfully");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_trusted_peer_failed()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::add_peer_failed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Failed to add trusted peer");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_remove_trusted_peer_failed()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::remove_peer_failed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Failed to remove trusted peer");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_friend_confirm()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::add_friend_confirm == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Friend added successfully");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_add_friend_failed()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::add_friend_failed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                METAX_NOTICE("Failed to add friend");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_user_public_key_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_public_key_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
                j->set("user_public_key",
                                std::string(in.message.get(), in.size));
                std::stringstream ss;
                j->stringify(ss);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                ss.str(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_user_keys_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_user_keys_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(0 != in.size);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                std::string(in.message.get(), in.size),
                                "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_user_keys_failed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_user_keys_failed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                poco_assert(nullptr != in.message.get());
                std::string err(in.message.get(), in.size);
                METAX_NOTICE(err);
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string m = R"({"error":")";
                m += err + R"("})";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                m, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_online_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_online_peers_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                in.message.get(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_pairing_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_pairing_peers_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                in.message.get(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_generated_code()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_generated_code == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                in.message.get(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_start_pairing_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::start_pairing_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                ims_response ir(content,
                        Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR,
                        in.message.get(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_request_keys_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::request_keys_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s =
                        R"({"KeyTransfer":"received keys successfully"})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                                        s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_request_keys_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::request_keys_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                std::string s =
                        R"({"error":")" + std::string(in.message.get(), in.size)
                                        + R"("})";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_metax_info_resposne()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_metax_info_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                in.message.get(), "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_set_metax_info_ok()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::set_metax_info_ok == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                R"({"status":"OK"})", "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_set_metax_info_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::set_metax_info_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                std::string m = R"({"status":"error","message":")" +
                        std::string(in.message.get(), in.size) +
                        "\"}";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                m, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_trusted_peer_list_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_peer_list_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_friend_list_response()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_friend_list_response == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = in.message.get();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_move_cache_to_storage_completed()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::move_cache_to_storage_completed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_NOTICE("Moving cache to storage completed");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);

                std::string s;
                Poco::Net::HTTPResponse::HTTPStatus c;
                if (in.status) {
                        s = R"({"success":)"
                                R"("Moving cache to storage completed"})";
                        c = Poco::Net::HTTPResponse::HTTP_OK;
                } else {
                        s = R"({"error":"Failed to move cache to storage. )"
                                R"(See metax logs."})";
                        c = Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
                }
                ims_response ir(content, c, s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
}

void leviathan::metax_web_api::web_api_adapter::
clean_get_request(metax::ID32 id, unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_get_requests_mutex);
        Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
        poco_assert(m_get_requests.end() != m_get_requests.find(id));
        m_get_requests.erase(id);
        m_client_to_request.erase(client_id);
        METAX_INFO("m_get_requests size = " +
                        platform::utils::to_string(m_get_requests.size()));
        METAX_INFO("m_client_to_request size = " +
                        platform::utils::to_string(m_client_to_request.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
clean_save_request(metax::ID32 id, unsigned long client_id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_save_requests_mutex);
        Poco::ScopedLock<Poco::Mutex> cm(m_client_to_request_mutex);
        poco_assert(m_save_requests.end() != m_save_requests.find(id));
        m_save_requests.erase(id);
        m_client_to_request.erase(client_id);
        METAX_INFO("m_save_requests size = " +
                        platform::utils::to_string(m_save_requests.size()));
        METAX_INFO("m_client_to_request size = " +
                        platform::utils::to_string(m_client_to_request.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
clean_config_request(metax::ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_config_requests_mutex);
        poco_assert(m_get_requests.end() != m_config_requests.find(id));
        m_config_requests.erase(id);
        METAX_INFO("m_config_requests size = " +
                        platform::utils::to_string(m_config_requests.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
clean_tmp_files(const std::string& p)
{
        Poco::File f(p);
        //Removes temporary created file
        if (f.exists()) {
                f.remove();
        }
}

void leviathan::metax_web_api::web_api_adapter::
send_streaming_chunk(request_info& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        ims_response ir(file, Poco::Net::HTTPResponse::HTTP_OK,
                        "", "");
        ir.response_file_path = in.response_file_path;
        poco_assert(req.m_stream_promises->size() > req.m_cur_stream_chunk);
        poco_assert(nullptr != req.m_stream_promises);
        (*req.m_stream_promises)[(unsigned int)req.m_cur_stream_chunk++].set_value(ir);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_complete_file(request_info& req)
{
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        METAX_NOTICE("Sending GET request response for "
                        + in.uuid.toString() + " uuid.");
        std::promise<ims_response>* pr = req.prom;
        poco_assert(nullptr != pr);
        ims_response ir(file, Poco::Net::HTTPResponse::HTTP_OK,
                        "", in.content_type);
        ir.response_file_path = in.response_file_path;
        pr->set_value(ir);
        Poco::Timestamp now;
        METAX_INFO("!test_log!get_request_response!time!"
                        + UTILS::to_string(now.epochMicroseconds()) + '!');
        clean_get_request(in.request_id, req.client_id);
}

void leviathan::metax_web_api::web_api_adapter::
send_get_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        poco_assert(m_get_requests.end() != i);
        if (nullptr != (i->second).m_stream_promises) {
                send_streaming_chunk(i->second);
        } else {
                send_complete_file(i->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_copy_failed_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::copy_failed == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                std::string s = "{\"error\":\"";
                s += in.message.get();
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                                s + "\"}", "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_recording_started()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::recording_started == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = R"({"uuid":")" + in.uuid.toString() + "\"}";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_recording_stopped()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::recording_stopped == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s =
                        R"({"status":"success"})";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_recording_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::recording_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = "";
                if (0 < in.size) {
                        s = R"({"error":")" +
                                std::string(in.message.get(), in.size) +
                                R"("})";
                } else {
                        s = R"({"error":"cannot start recording for )"
                                + (i->second).uuid.toString()
                                + R"("})";
                }
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_copy_succeeded_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::copy_succeeded == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(nullptr != in.message.get());
                std::string s = "{\"orig_uuid\":\"";
                s += in.uuid.toString() + "\", \"copy_uuid\":\""
                        + in.message.get() + "\"}";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
generate_promise_future_vectors_for_streaming(request_info& req,
                                                ims_response& ir)
{
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        METAX_NOTICE("Sending GET request response for "
                        + req.uuid.toString() + " uuid");
        req.m_stream_promises =
                new std::vector<std::promise<ims_response>>(
                                (unsigned int)in.chunk_count);
        ir.stream_futures.reset(new std::vector<std::future<ims_response>>);
        for (auto& p : *req.m_stream_promises) {
                ir.stream_futures->push_back(p.get_future());
        }
}

void leviathan::metax_web_api::web_api_adapter::
handle_streaming_prepare()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(metax::get == (i->second).command);
                ims_response ir(media_streaming,
                                Poco::Net::HTTPResponse::HTTP_OK,
                                "", in.content_type);
                ir.range.first = in.get_range.first;
                ir.range.second = in.get_range.second;
                ir.content_length = in.content_length;
                generate_promise_future_vectors_for_streaming(i->second, ir);
                poco_assert(nullptr != pr);
                pr->set_value(ir);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_on_update_registered()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::on_update_registered == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_NOTICE("Registered listener for "
                                + in.uuid.toString() + " uuid successfully");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = R"({"success": "registered listener for )";
                s += i->second.uuid.toString();
                s +=        R"("})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_on_update_register_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::on_update_register_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_ERROR("Failed to register listener for "
                                + in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s =
                        R"({"error": "Failed to register listener for: )";
                s += i->second.uuid.toString() + "." + in.message.get();
                s +=        R"("})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_on_update_unregister_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::on_update_unregister_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                poco_assert(nullptr != in.message.get());
                METAX_ERROR(in.message.get());
                std::string s = R"({"error":")";
                s += in.message.get();
                s += R"("})";
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_on_update_unregistered()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::on_update_unregistered == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_NOTICE("listener unregistered for " + in.uuid.toString());
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = R"({"success": "listener unregistered for )";
                s += i->second.uuid.toString();
                s +=        R"("})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_accept_share_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::accept_share_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_ERROR("Fail to accept share for " +
                                in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = R"({"error": ")";
                s += in.message.get();
                s += R"("})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_ERROR("Fail to get " + in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(metax::get_fail == in.cmd);
                poco_assert(nullptr != in.message.get());
                std::string m = R"({"error":")";
                m += in.message.get();
                m += R"("})";
                poco_assert(nullptr != pr);
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                                m, "application/json");
                METAX_NOTICE("Failed to get " + in.uuid.toString() + " uuid");
                pr->set_value(ir);
                Poco::Timestamp now;
                METAX_INFO("!test_log!get_request_response!time!" +
                              UTILS::to_string(now.epochMicroseconds()) + '!');
                clean_get_request(in.request_id, (i->second).client_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_delete_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::delete_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_NOTICE("Fail to delete " + in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                std::string m = R"({"error":")";
                m += in.message.get();
                m += R"("})";
                poco_assert(nullptr != pr);
                ims_response ir(content,
                                Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                                m, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id, (i->second).client_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_update_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::update_fail == in.cmd || metax::save_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_ERROR("Fail to update " + in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                std::string m = R"({"error":")";
                m += in.message;
                m += R"("})";
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                                m, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_livestream_cancel_to_client(const request_info& req)
{
        METAX_TRACE(__FUNCTION__);
        std::string m = R"({"error":")";
        std::promise<ims_response>* pr = nullptr;
        if (nullptr == req.m_stream_promises) {
                m += (*metax_rx).message;
                pr = req.prom;
        } else {
                m += "livestreaming finished";
                poco_assert(1 == req.m_stream_promises->size());
                pr = &((*req.m_stream_promises)[0]);
        }
        poco_assert(nullptr != pr);
        m += R"("})";
        ims_response ir(content, Poco::Net::HTTPResponse::HTTP_GONE,
                        m, "application/json");
        pr->set_value(ir);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_save_stream_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::save_stream_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_ERROR("Failed to save livestream");
                send_livestream_cancel_to_client(i->second);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_deleted_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::deleted == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                //poco_assert(metax::del == (i->second).cmd);
                std::promise<ims_response>* pr = (i->second).prom;
                std::string m = R"({"deleted":")";
                m += in.uuid.toString();
                m += R"("})";
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                m, "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_dump_user_info_succeeded()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::dump_user_info_succeeded == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                poco_assert(metax::dump_user_info == (i->second).command);
                std::promise<ims_response>* pr = (i->second).prom;
                Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
                obj->set("status", "succeeded");
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                platform::utils::to_string(obj),
                                "application/json");
                pr->set_value(ir);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_regenerate_user_keys_succeeded()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::regenerate_user_keys_succeeded == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                poco_assert(metax::regenerate_user_keys == (i->second).command);
                std::promise<ims_response>* pr = (i->second).prom;
                Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
                obj->set("status", "succeeded");
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                platform::utils::to_string(obj),
                                "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_regenerate_user_keys_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::regenerate_user_keys_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_config_requests_mutex);
                i = m_config_requests.find(in.request_id);
        }
        if (m_config_requests.end() != i) {
                poco_assert(metax::regenerate_user_keys == (i->second).command);
                std::promise<ims_response>* pr = (i->second).prom;
                Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
                poco_assert(nullptr != in.message.get());
                obj->set("error", in.message.get());
                poco_assert(nullptr != pr);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                platform::utils::to_string(obj),
                                "application/json");
                pr->set_value(ir);
                clean_config_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_get_livestream_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(metax::get_livestream_fail == in.cmd);
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                METAX_NOTICE("Livestream is finished");
                send_livestream_cancel_to_client(i->second);
                clean_get_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_share_failed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = construct_share_error_message();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax_web_api::web_api_adapter::
construct_share_error_message()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        std::string s;
        if (m_save_requests.end() != i) {
                std::string err(in.message, in.size);
                s = (metax::share_fail == in.cmd) ? "{" :
                        R"({"share_uuid": ")" + (i->second).uuid.toString() + R"(",)";
                s += (metax::deliver_fail == in.cmd) ?
                        R"("warning":")" : R"("error":")";
                s += err + R"( for the request with )";
                s += i->second.username.empty() ? "public key" :
                        i->second.username;
                s += R"("})";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return s;
}

void leviathan::metax_web_api::web_api_adapter::
construct_send_error_message(std::string& s)
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                std::string err(in.message, in.size);
                s += (metax::deliver_fail == in.cmd) ?
                        R"({"warning":")" : R"({"error":")";
                s += err + R"( for the request with )";
                s += i->second.username.empty() ? "public key" :
                        i->second.username;
                s += R"("})";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_send_to_peer_failed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_ERROR("Fail to send data to peer");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string err(in.message, in.size);
                METAX_ERROR(err);
                std::string s;
                construct_send_error_message(s);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_save_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_NOTICE("Received save confirmed response for "
                                + in.uuid.toString() + " uuid");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::stringstream o;
                Poco::JSON::Object res;
                poco_assert(! in.uuid.isNull());
                res.set("uuid", in.uuid.toString());
                res.stringify(o);
                std::string s = o.str();
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                if ("" != (i->second).info) {
                        clean_tmp_files((i->second).info);
                }
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_save_stream_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                request_info& req = i->second;
                std::promise<ims_response>* pr = req.prom;
                poco_assert(nullptr != pr);
                std::stringstream o;
                Poco::JSON::Object res;
                poco_assert(! in.uuid.isNull());
                res.set("uuid", in.uuid.toString());
                res.stringify(o);
                std::string s = o.str();
                ims_response ir(livestream_start, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                req.m_stream_promises =
                        new std::vector<std::promise<ims_response>>(1);
                // may be this should be replaced by thread save queue
                // usage
                ir.stream_futures.reset(new std::vector<std::future<ims_response>>);
                ir.stream_futures->push_back((*req.m_stream_promises)[0].get_future());
                pr->set_value(ir);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_livestream_content()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                poco_assert(0 != in.size);
                std::string s(in.message.get(), in.size);
                request_info& req = i->second;
                ims_response ir(livestream_content,
                                        Poco::Net::HTTPResponse::HTTP_OK,
                                        s, "video/mp2t");
                if (nullptr == req.m_stream_promises) {
                        std::promise<ims_response>* pr = req.prom;
                        req.m_stream_promises =
                                new std::vector<std::promise<ims_response>>(1);
                        // may be this should be replaced by thread save queue
                        // usage
                        ir.stream_futures.reset(new std::vector<std::future<ims_response>>);
                        ir.stream_futures->push_back(((*(req.m_stream_promises))[0]).get_future());
                        pr->set_value(ir);
                } else {
                        poco_assert(1 == req.m_stream_promises->size());
                        std::promise<ims_response> pr = std::move((*req.m_stream_promises)[0]);
                        req.m_stream_promises->pop_back();
                        req.m_stream_promises->push_back(std::promise<ims_response>());
                        // may be this should be replaced by thread save queue
                        // usage
                        ir.stream_futures.reset(new std::vector<std::future<ims_response>>);
                        ir.stream_futures->push_back(((*(req.m_stream_promises))[0]).get_future());
                        pr.set_value(ir);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_share_confirm()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_NOTICE(in.uuid.toString()
                                + " uuid is successfully shared");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(metax::share == (i->second).command);
                Poco::JSON::Object::Ptr j = new Poco::JSON::Object();
                j->set("share_uuid", in.uuid.toString());
                j->set("key", in.aes_key);
                j->set("iv", in.aes_iv);
                std::stringstream ss;
                j->stringify(ss);
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                ss.str(), "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_share_accepted()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_NOTICE(in.uuid.toString() +
                                " shared uuid is successfully accepted");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                poco_assert(metax::accept_share == (i->second).command);
                std::string s = R"({"share":"accepted"})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_send_to_peer()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        poco_assert(nullptr != in.message.get());
        std::string msg(in.message.get(), in.size);
        Poco::ScopedLock<Poco::Mutex> wm(m_wsockets_mutex);
        if (m_wsockets.empty()) {
                send_deliver_fail_response_to_metax();
        } else {
                deliver_notification(msg);
        }
        add_message_to_queue(msg);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_deliver_fail_response_to_metax()
{
        METAX_TRACE(__FUNCTION__);
        METAX_WARNING("Received data from a peer, "
                        "but websocket is not connected.");
        Poco::ScopedLock<Poco::Mutex> m(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::deliver_fail;
        out.request_id = (*metax_rx).request_id;
        out.set_payload(std::string("Failed to deliver data to peer"));
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
deliver_notification(const std::string& msg)
{
        METAX_TRACE(__FUNCTION__);
        send_websocket_message(msg);
        Poco::ScopedLock<Poco::Mutex> m(m_metax_tx_mutex);
        metax::ims_kernel_package& out = *metax_tx;
        out.cmd = metax::send_to_peer_confirm;
        out.request_id = (*metax_rx).request_id;
        metax_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
add_message_to_queue(const std::string& msg)
{
        METAX_TRACE(__FUNCTION__);
        m_last_messages.push_back(msg);
        if (10 < m_last_messages.size()) {
                m_last_messages.pop_front();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_send_to_peer_confirm()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        if (m_save_requests.end() != i) {
                METAX_NOTICE("Data is delivered successfully");
                std::promise<ims_response>* pr = (i->second).prom;
                poco_assert(nullptr != pr);
                std::string s = "{";
                if (metax::share == (i->second).command) {
                        s += R"("share_uuid":")" + (i->second).uuid.toString()
                                + R"(",)";
                }
                s += R"("msg":"Data is delivered successfully"})";
                ims_response ir(content, Poco::Net::HTTPResponse::HTTP_OK,
                                s, "application/json");
                pr->set_value(ir);
                clean_save_request(in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_send_share_failed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
        }
        switch ((i->second).command) {
                case metax::share: {
                        handle_share_failed();
                        break;
                } case metax::send_to_peer: {
                        handle_send_to_peer_failed();
                        break;
                } default : {
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_sync_finished()
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> gm(m_wsockets_mutex);
        std::string m = R"({"event":"sync finished"})";
        if (m_wsockets.empty()) {
                METAX_WARNING(
                        "No websocket connection to inform that sync finished");
        } else {
                send_websocket_message(std::string(m));
        }
        add_message_to_queue(m);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
handle_metax_fail()
{
        METAX_TRACE(__FUNCTION__);
        const metax::ims_kernel_package& in = *metax_rx;
        std::promise<ims_response>* pr = nullptr;
        std::string m = R"({"error":"failed to process request"})";
        std::map<metax::ID32, request_info>::iterator i;
        {
                Poco::ScopedLock<Poco::Mutex> gm(m_get_requests_mutex);
                i = m_get_requests.find(in.request_id);
        }
        if (m_get_requests.end() != i) {
                pr = (i->second).prom;
                clean_get_request(in.request_id, (i->second).client_id);
        } else {
                Poco::ScopedLock<Poco::Mutex> sm(m_save_requests_mutex);
                i = m_save_requests.find(in.request_id);
                if (m_save_requests.end() != i) {
                        pr = (i->second).prom;
                        clean_save_request(in.request_id);
                } else {
                        Poco::ScopedLock<Poco::Mutex> cm(m_config_requests_mutex);
                        i = m_config_requests.find(in.request_id);
                        poco_assert(m_config_requests.end() != i);
                        pr = (i->second).prom;
                        clean_config_request(in.request_id);
                }
        }
        ims_response ir(content, Poco::Net::HTTPResponse::HTTP_NOT_FOUND,
                                                        m, "application/json");
        pr->set_value(ir);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
send_websocket_message(const std::string& m)
{
        auto wi = m_wsockets.begin();
        while (wi != m_wsockets.end()) {
                poco_assert(nullptr != *wi);
                METAX_TRACE("sending message to websocket: " +
                                                (*wi)->peer_address);
                bool b = (*wi)->send(m);
                if (! b) {
                        wi = m_wsockets.erase(wi);
                } else {
                        ++wi;
                }
        }
}

void leviathan::metax_web_api::web_api_adapter::
handle_metax_input()
{
        if (! metax_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_metax_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        metax_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
process_metax_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax_rx.has_data());
        const metax::ims_kernel_package& in = *metax_rx;
        switch (in.cmd) {
                case metax::get_data : {
                        send_get_response();
                        break;
                } case metax::updated :
                case metax::save_confirm : {
                        send_save_response();
                        break;
                } case metax::save_stream_offer : {
                        send_save_stream_response();
                        break;
                } case metax::livestream_content : {
                        send_livestream_content();
                        break;
                } case metax::share_confirm : {
                        handle_share_confirm();
                        break;
                } case metax::send_to_peer : {
                        handle_send_to_peer();
                        break;
                } case metax::send_to_peer_confirm : {
                        handle_send_to_peer_confirm();
                        break;
                } case metax::send_to_fail: {
                } case metax::deliver_fail: {
                } case metax::share_fail: {
                        handle_send_share_failed();
                        break;
                } case metax::get_fail: {
                        handle_get_fail();
                        break;
                } case metax::save_fail:
                  case metax::update_fail: {
                        handle_update_fail();
                        break;
                } case metax::save_stream_fail: {
                        handle_save_stream_fail();
                        break;
                } case metax::get_friend_found : {
                        handle_get_friend_confirm();
                        break;
                } case metax::get_friend_not_found : {
                        handle_get_friend_not_found();
                        break;
                } case metax::add_peer_confirm : {
                        handle_add_trusted_peer_confirm();
                        break;
                } case metax::add_peer_failed : {
                        handle_add_trusted_peer_failed();
                        break;
                } case metax::remove_peer_confirm : {
                        handle_remove_trusted_peer_confirm();
                        break;
                } case metax::remove_peer_failed : {
                        handle_remove_trusted_peer_failed();
                        break;
                } case metax::add_friend_confirm : {
                        handle_add_friend_confirm();
                        break;
                } case metax::add_friend_failed : {
                        handle_add_friend_failed();
                        break;
                } case metax::get_public_key_response : {
                        handle_get_user_public_key_response();
                        break;
                } case metax::get_user_keys_response : {
                        handle_get_user_keys_response();
                        break;
                } case metax::get_user_keys_failed : {
                        handle_get_user_keys_failed();
                        break;
                } case metax::get_peer_list_response : {
                        handle_get_trusted_peer_list_response();
                        break;
                } case metax::get_friend_list_response : {
                        handle_get_friend_list_response();
                        break;
                } case metax::move_cache_to_storage_completed : {
                        handle_move_cache_to_storage_completed();
                        break;
                } case metax::prepare_streaming: {
                        handle_streaming_prepare();
                        break;
                } case metax::on_update_registered: {
                        handle_on_update_registered();
                        break;
                } case metax::on_update_register_fail: {
                        handle_on_update_register_fail();
                        break;
                } case metax::on_update_unregistered: {
                        handle_on_update_unregistered();
                        break;
                } case metax::on_update_unregister_fail: {
                        handle_on_update_unregister_fail();
                        break;
                } case metax::accept_share_fail: {
                        handle_accept_share_fail();
                        break;
                } case metax::share_accepted: {
                        handle_share_accepted();
                        break;
                } case metax::get_livestream_fail: {
                        handle_get_livestream_fail();
                        break;
                } case metax::deleted: {
                        handle_deleted_response();
                        break;
                } case metax::delete_fail: {
                        handle_delete_fail();
                        break;
                } case metax::get_online_peers_response: {
                        handle_get_online_peers_response();
                        break;
                } case metax::get_pairing_peers_response: {
                        handle_get_pairing_peers_response();
                        break;
                } case metax::get_generated_code: {
                        handle_get_generated_code();
                        break;
                } case metax::start_pairing_fail: {
                        handle_start_pairing_fail();
                        break;
                } case metax::request_keys_response: {
                        handle_request_keys_response();
                        break;
                } case metax::request_keys_fail: {
                        handle_request_keys_fail();
                        break;
                } case metax::get_metax_info_response: {
                        handle_get_metax_info_resposne();
                        break;
                } case metax::set_metax_info_ok: {
                        handle_set_metax_info_ok();
                        break;
                } case metax::set_metax_info_fail: {
                        handle_set_metax_info_fail();
                        break;
                } case metax::copy_succeeded: {
                        handle_copy_succeeded_response();
                        break;
                } case metax::copy_failed: {
                        handle_copy_failed_response();
                        break;
                } case metax::recording_started: {
                        handle_recording_started();
                        break;
                } case metax::recording_stopped: {
                        handle_recording_stopped();
                        break;
                } case metax::recording_fail: {
                        handle_recording_fail();
                        break;
                } case metax::sync_finished: {
                        handle_sync_finished();
                        break;
                } case metax::dump_user_info_succeeded: {
                        handle_dump_user_info_succeeded();
                        break;
                } case metax::regenerate_user_keys_succeeded: {
                        handle_regenerate_user_keys_succeeded();
                        break;
                } case metax::regenerate_user_keys_fail: {
                        handle_regenerate_user_keys_fail();
                        break;
                } case metax::failed: {
                        handle_metax_fail();
                        break;
                } default : {
                        METAX_WARNING("This case is not handled yet! Command="
                                        + platform::utils::to_string(in.cmd));
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
set_exception_to_promises(std::map<metax::ID32, request_info>& reqs)
{
        METAX_TRACE(__FUNCTION__);
        std::exception_ptr w = std::make_exception_ptr(
                        std::runtime_error("Server is gone offline."));
        for (auto& i : reqs) {
                try {
                        auto& req = i.second;
                        if (nullptr != req.m_stream_promises) {
                                unsigned int curr =
                                        (unsigned int)req.m_cur_stream_chunk;
                                // curr can be equal to size here if
                                // audio/video content has been sent completely
                                // but clean request isn't yet received from
                                // from db_request_handler
                                if ((*req.m_stream_promises).size() > curr ) {
                                        (*req.m_stream_promises)[curr].set_exception(w);
                                }
                        } else {
                                poco_assert(nullptr != req.prom);
                                req.prom->set_exception(w);
                        }
                } catch (...) {
                        // nothing to do.
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax_web_api::web_api_adapter::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        while(true) {
                if (! wait()) {
                        break;
                }
                handle_metax_input();
        }
        set_exception_to_promises(m_save_requests);
        set_exception_to_promises(m_get_requests);
        set_exception_to_promises(m_config_requests);
        // Wait for all request handleres to finish.
        Poco::ThreadPool& dp = Poco::ThreadPool::defaultPool();
        dp.joinAll();
        finish();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (const Poco::Exception& e) {
        METAX_ERROR(e.displayText());
} catch (const std::exception& e) {
        METAX_ERROR(e.what());
} catch (...) {
        METAX_ERROR("Unhandled exception !!!");
}

leviathan::metax_web_api::web_api_adapter::
web_api_adapter()
        : platform::csp::task("web_api_adapter", Poco::Logger::get("metax_web_api.web_api_adapter"))
        , metax_rx(this)
        , metax_tx(this)
        , m_enable_non_localhost_save(false)
        , m_enable_non_localhost_get(true)
        , m_get_requests_mutex()
        , m_save_requests_mutex()
        , m_config_requests_mutex()
        , m_metax_tx_mutex()
        , m_counter(0)
        , m_get_requests()
        , m_save_requests()
        , m_config_requests()
        , m_wsockets()
        , m_wsockets_mutex()
        , m_client_to_request()
        , m_client_to_request_mutex()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax_web_api::web_api_adapter::
~web_api_adapter()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

