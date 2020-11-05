/**
 * @file src/metax_web_api/configuration_handler.cpp
 *
 * @brief Implementation of the class
 * @ref leviathan::metax_web_api::configuration_handler.
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
#include "configuration_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include "Poco/Net/HTMLForm.h"
#include "Poco/NumberParser.h"

// Headers from standard C++ libraries
#include <iostream>

const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG = "/config";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_ADD_FRIEND = "/add_friend";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_FRIEND_LIST = "/get_friend_list";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_ADD_PEER = "/add_trusted_peer";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_REMOVE_PEER = "/remove_trusted_peer";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_PEER_LIST = "/get_trusted_peer_list";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_USER_PUBLIC_KEY = "/get_user_public_key";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_USER_KEYS = "/get_user_keys";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_ONLINE_PEERS = "/get_online_peers";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_METAX_INFO = "/get_metax_info";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_SET_METAX_INFO = "/set_metax_info";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_RECONNECT_TO_PEERS = "/reconnect_to_peers";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_START_PAIRING = "/start_pairing";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_CANCEL_PAIRING = "/cancel_pairing";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_GET_PAIRING_PEERS = "/get_pairing_peers";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_CONNECT_SERVER = "/request_keys";
const std::string leviathan::metax_web_api::configuration_handler::
URI_CONFIG_REGENERATE_KEYS = "/regenerate_user_keys";

void leviathan::metax_web_api::configuration_handler::
handleRequest(Poco::Net::HTTPServerRequest& req,
        Poco::Net::HTTPServerResponse& res)
try {
        METAX_INFO("Received configuration request from  "
                        + req.clientAddress().toString());
        std::string uri = req.getURI();
        const std::string req_uri = uri.substr(URI_CONFIG.size(), uri.size());
        std::promise<web_api_adapter::ims_response> pr;
        std::future<web_api_adapter::ims_response> fu = pr.get_future();
        set_response_header(&res);
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        if (URI_CONFIG_ADD_PEER ==
                        req_uri.substr(0, URI_CONFIG_ADD_PEER.size())) {
                handle_add_trusted_peer_request(req, pr);
        } else if (URI_CONFIG_REMOVE_PEER ==
                        req_uri.substr(0, URI_CONFIG_REMOVE_PEER.size())) {
                handle_remove_trusted_peer_request(req, pr);
        } else if (URI_CONFIG_GET_PEER_LIST ==
                        req_uri.substr(0, URI_CONFIG_GET_PEER_LIST.size())) {
                ims->handle_get_trusted_peer_list_request(&pr);
        } else if (URI_CONFIG_ADD_FRIEND ==
                        req_uri.substr(0, URI_CONFIG_ADD_FRIEND.size())) {
                handle_add_friend(req, pr);
        } else if (URI_CONFIG_GET_FRIEND_LIST ==
                     req_uri.substr(0, URI_CONFIG_GET_FRIEND_LIST.size())) {
                ims->handle_get_friend_list_request(&pr);
        } else if (URI_CONFIG_GET_USER_PUBLIC_KEY ==
                     req_uri.substr(0, URI_CONFIG_GET_USER_PUBLIC_KEY.size())) {
                ims->handle_get_user_public_key_request(&pr);
        } else if (URI_CONFIG_GET_USER_KEYS ==
                     req_uri.substr(0, URI_CONFIG_GET_USER_KEYS.size())) {
                handle_get_user_keys_request(req, pr);
        } else if (URI_CONFIG_GET_ONLINE_PEERS ==
                     req_uri.substr(0, URI_CONFIG_GET_ONLINE_PEERS.size())) {
                ims->handle_get_online_peers(&pr);
        } else if (URI_CONFIG_GET_METAX_INFO ==
                     req_uri.substr(0, URI_CONFIG_GET_METAX_INFO.size())) {
                ims->handle_get_metax_info(&pr);
        } else if (URI_CONFIG_SET_METAX_INFO ==
                     req_uri.substr(0, URI_CONFIG_SET_METAX_INFO.size())) {
                handle_set_metax_info(req, pr);
        } else if (URI_CONFIG_RECONNECT_TO_PEERS ==
                     req_uri.substr(0, URI_CONFIG_RECONNECT_TO_PEERS.size())) {
                ims->handle_reconnect_to_peers(&pr);
        } else if (URI_CONFIG_START_PAIRING ==
                     req_uri.substr(0, URI_CONFIG_START_PAIRING.size())) {
                handle_start_pairing(req, pr);
        } else if (URI_CONFIG_CANCEL_PAIRING ==
                     req_uri.substr(0, URI_CONFIG_CANCEL_PAIRING.size())) {
                ims->handle_cancel_pairing(&pr);
        } else if (URI_CONFIG_GET_PAIRING_PEERS ==
                     req_uri.substr(0, URI_CONFIG_GET_PAIRING_PEERS.size())) {
                ims->handle_get_pairing_peers(&pr);
        } else if (URI_CONFIG_CONNECT_SERVER ==
                     req_uri.substr(0, URI_CONFIG_CONNECT_SERVER.size())) {
                handle_request_keys(req, pr);
        } else if (URI_CONFIG_REGENERATE_KEYS ==
                     req_uri.substr(0, URI_CONFIG_REGENERATE_KEYS.size())) {
                handle_regenerate_user_keys(req, pr);
        } else {
                throw Poco::Exception("Unknown config request");
        }
        wait_for_response(fu, res);
        METAX_INFO("Response to configuration request sent to  "
                        + req.clientAddress().toString());
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
        std::string j = R"({"error": ")" + e.displayText() + R"("})";
        res.setStatus(Poco::Net::HTTPServerResponse::HTTP_BAD_REQUEST);
        res.sendBuffer(j.c_str(), j.size());
} catch (std::exception& e) {
        METAX_WARNING(e.what());
        std::string j = R"({"error": ")";
        j += e.what();
        j += R"("})";
        res.setStatus(Poco::Net::HTTPServerResponse::HTTP_BAD_REQUEST);
        res.sendBuffer(j.c_str(), j.size());
} catch (...) {
        METAX_WARNING("Bad Request");
        std::string j = R"({"error": "Bad Request"})";
        res.setStatus(Poco::Net::HTTPServerResponse::HTTP_BAD_REQUEST);
        res.sendBuffer(j.c_str(), j.size());
}

void leviathan::metax_web_api::configuration_handler::
handle_add_trusted_peer_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req, req.stream());
        if (! f.has("key")) {
                throw Poco::Exception("key must be specified");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        std::string k = f.get("key");
        platform::utils::trim(k);
        if ("" == k) {
                throw Poco::Exception("key contains empty value");
        }
        ims->handle_add_trusted_peer_request(&pr, k);
}

void leviathan::metax_web_api::configuration_handler::
handle_remove_trusted_peer_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req, req.stream());
        if (! f.has("key")) {
                throw Poco::Exception("key must be specified");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_remove_trusted_peer_request(&pr, f.get("key"));
}

void leviathan::metax_web_api::configuration_handler::
handle_add_friend(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req, req.stream());
        if (! f.has("key") || ! f.has("username")) {
                throw Poco::Exception("username and key must be specified");
        }
        std::string k = f.get("key");
        std::string u = f.get("username");
        platform::utils::trim(k);
        platform::utils::trim(u);
        if ("" == k || "" == u) {
                throw Poco::Exception("key or username contains empty value");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_add_friend_request(&pr, u, k);
}

void leviathan::metax_web_api::configuration_handler::
handle_set_metax_info(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        if (! req.clientAddress().host().isLoopback()) {
                throw Poco::Exception("set_metax_info request only allowed from"
                                " localhost machine.");
        }
        Poco::Net::HTMLForm f(req, req.stream());
        if (f.empty() || ! f.has("metax_user_uuid")) {
                throw Poco::Exception("only metax_user_uuid parameter is "
                                "supported now");
        }
        std::string uuid = f.get("metax_user_uuid");
        Poco::UUID u;
        if (! u.tryParse(uuid)) {
                throw Poco::Exception("uuid is not valid");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_set_metax_info(&pr, uuid);
}

void leviathan::metax_web_api::configuration_handler::
handle_get_user_keys_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        if (! req.clientAddress().host().isLoopback()) {
                throw Poco::Exception("get_user_keys request only allowed from"
                                " localhost machine.");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_get_user_keys_request(&pr);
}

void leviathan::metax_web_api::configuration_handler::
handle_start_pairing(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        if (! req.clientAddress().host().isLoopback()) {
                throw Poco::Exception("start_pairing request only allowed from"
                                " localhost machine.");
        }
        Poco::Net::HTMLForm f(req, req.stream());
        if (f.empty() || ! f.has("timeout")) {
                throw Poco::Exception("only timeout parameter is supported now");
        }
        std::string timeout = f.get("timeout");
        unsigned t;
        if (! Poco::NumberParser::tryParseUnsigned(timeout, t)) {
                throw Poco::Exception("timeout is not valid");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_start_pairing(&pr, timeout);
}

void leviathan::metax_web_api::configuration_handler::
handle_request_keys(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        if (! req.clientAddress().host().isLoopback()) {
                throw Poco::Exception("request_keys request only allowed from"
                                " localhost machine.");
        }
        Poco::Net::HTMLForm f(req, req.stream());
        if (f.empty() || ! f.has("ip") || ! f.has("code")) {
                throw Poco::Exception("only ip and code parameters are supported now");
        }
        std::string ip = f.get("ip");
        Poco::Net::IPAddress i;
        if (! Poco::Net::IPAddress::tryParse(ip, i)) {
                throw Poco::Exception("ip is not valid");
        }
        std::string code = f.get("code");
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_request_keys(&pr, ip, code);
}

void leviathan::metax_web_api::configuration_handler::
handle_regenerate_user_keys(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        if (! req.clientAddress().host().isLoopback()) {
                throw Poco::Exception("regenerate keys request only allowed "
                                "from localhost machine.");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_regenerate_user_keys(&pr);
}

void leviathan::metax_web_api::configuration_handler::
set_response_header(Poco::Net::HTTPServerResponse* r)
{
        r->set("Cache-Control", "no-cache, no-store, must-revalidate");
        r->set("Access-Control-Allow-Origin", "*");
        r->set("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
        r->set("Access-Control-Max-Age", "1000");
        r->set("Access-Control-Allow-Headers", "Content-Type");
}

void leviathan::metax_web_api::configuration_handler::
wait_for_response(std::future<web_api_adapter::ims_response>& fu,
                Poco::Net::HTTPServerResponse& res)
{
        web_api_adapter::ims_response ir = fu.get();
        res.setStatus(ir.st);
        res.sendBuffer(ir.info.c_str(), ir.info.size());
}

std::string leviathan::metax_web_api::configuration_handler::
name() const
{
        return "metax_web_api.configuration_handler";
}

leviathan::metax_web_api::configuration_handler::
configuration_handler()
        : m_logger(Poco::Logger::get("metax_web_api.configuration_handler"))
{
}

leviathan::metax_web_api::configuration_handler::
~configuration_handler()
{
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

