/**
 * @file src/metax_web_api/db_request_handler.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::db_request_handler.
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
#include "db_request_handler.hpp"
#include "web_api_adapter.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/StreamCopier.h>
#include <Poco/TemporaryFile.h>

// Headers from standard libraries
#include <iostream>
#include <fstream>

const std::string leviathan::metax_web_api::db_request_handler::
URI_DB = "/db";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_GET = "/get";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SAVE = "/save";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_COPY = "/copy";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SAVE_DATA = "/data";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SAVE_PATH = "/path";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SAVE_NODE = "/node";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SAVE_STREAM = "/stream";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SHARE = "/share";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_ACCEPT_SHARE = "/accept_share";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_SEND_TO = "/sendto";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_MV_CACHE_TO_STORAGE = "/move_cache_to_storage";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_REGISTER_LISTENER = "/register_listener";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_UNREGISTER_LISTENER = "/unregister_listener";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_DELETE = "/delete";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_RECORDING = "/recording";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_RECORDING_START = "/start";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_RECORDING_STOP = "/stop";
const std::string leviathan::metax_web_api::db_request_handler::
URI_DB_DUMP_USER_INFO = "/dump_user_info";

leviathan::metax_web_api::URI leviathan::metax_web_api::db_request_handler::
get_request_type(const std::string& u)
{
        if (URI_DB_SAVE_NODE == u.substr(URI_DB_SAVE.size(),
                                URI_DB_SAVE_NODE.size())) {
                return save_node;
        } else if (URI_DB_SAVE_PATH == u.substr(URI_DB_SAVE.size(),
                                URI_DB_SAVE_PATH.size())) {
                return save_path;
        } else if (URI_DB_SAVE_DATA == u.substr(URI_DB_SAVE.size(),
                                URI_DB_SAVE_DATA.size())) {
                return save_data;
        } else if (URI_DB_SEND_TO == u.substr(URI_DB_SEND_TO.size(),
                                URI_DB_SEND_TO.size())) {
        } else if (URI_DB_SAVE_STREAM == u.substr(URI_DB_SAVE.size(),
                                URI_DB_SAVE_STREAM.size())) {
                return save_stream;
        }
        return none;
}


leviathan::metax::ims_kernel_package::save_option
leviathan::metax_web_api::db_request_handler::
get_save_option(const Poco::Net::HTMLForm& f) const
{
        if (f.has("local")) {
                std::string v = f.get("local");
                if ("1" == v) {
                        return metax::ims_kernel_package::LOCAL;
                } else if ("0" == v) {
                        return metax::ims_kernel_package::NO_LOCAL;
                } else {
                        throw Poco::Exception("Invalid parameter: local=" + v);
                }
        }
        return metax::ims_kernel_package::DEFAULT;
}

void leviathan::metax_web_api::db_request_handler::
parse_expire_date(Poco::Net::HTMLForm& f, Poco::Int64& e)
{
        e = DEFAULT_EXPIRE_DATE;
        if (f.has("expire_date")) {
                std::istringstream iss(f.get("expire_date"));
                iss >> e;
                // TODO - do proper error handling.
                if (iss.fail()) {
                        throw Poco::Exception(
                                        "Invalid 'expire_date' option.");
                }
        }
}

bool leviathan::metax_web_api::db_request_handler::
handle_web_save_request(Poco::Net::HTTPServerRequest& request,
                   URI u, std::string& s, Poco::UUID& uuid,
                   metax::ims_kernel_package::save_option& l, Poco::Int64& e)
{
        switch (u) {
                case metax_web_api::save_node :
                case metax_web_api::save_path : {
                        Poco::StreamCopier::copyToString(request.stream(), s);
                        Poco::Net::HTMLForm f(request, request.stream());
                        if (f.has("id")) {
                                uuid.parse(f.get("id"));
                        }
                        parse_expire_date(f, e);
                        l = get_save_option(f);
                        return ! f.has("enc") || "1" == f.get("enc");
              } case  metax_web_api::save_data : {
                        upload_handler uh;
                        Poco::Net::HTMLForm f(request, request.stream(), uh);
                        if (f.has("id")) {
                                uuid.parse(f.get("id"));
                        }
                        parse_expire_date(f, e);
                        s = uh.get_upload_file_path();
                        l = get_save_option(f);
                        return ! f.has("enc") || "1" == f.get("enc");
              } case  metax_web_api::save_stream : {
                        Poco::Net::HTMLForm f(request, request.stream());
                        if (f.has("url")) {
                                 s = f.get("url");
                        }
                        if (f.has("id")) {
                                uuid.parse(f.get("id"));
                        }
                        l = get_save_option(f);
                        return ! f.has("enc") || "1" == f.get("enc");
              } default : {
                        throw Poco::Exception("Requested url is not handled yet.");
              }

        }
        return true;
}

void leviathan::metax_web_api::db_request_handler::
handleRequest(Poco::Net::HTTPServerRequest& req,
                Poco::Net::HTTPServerResponse& res)
try {
        METAX_INFO("Received database request from  "
                        + req.clientAddress().toString());
        std::string uri = req.getURI();
        const std::string req_uri = uri.substr(URI_DB.size(), uri.size());
        std::promise<web_api_adapter::ims_response> pr;
        std::future<web_api_adapter::ims_response> fu = pr.get_future();
        set_response_headers(&res);
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        if ("OPTIONS" == req.getMethod()) {
                res.send();
                return;
        } else if (URI_DB_GET == req_uri.substr(0, URI_DB_GET.size())) {
                handle_get_request(req, pr);
        } else if (URI_DB_SAVE == req_uri.substr(0, URI_DB_SAVE.size())) {
                handle_save_request(req, req_uri, pr);
        } else if (URI_DB_RECORDING ==
                        req_uri.substr(0, URI_DB_RECORDING.size())) {
                handle_recording_request(req, pr, req_uri);
        } else if (URI_DB_COPY == req_uri.substr(0, URI_DB_COPY.size())) {
                handle_copy_request(req, pr);
        } else if (URI_DB_SHARE == req_uri.substr(0, URI_DB_SHARE.size())) {
                handle_share_request(req, pr);
        } else if (URI_DB_ACCEPT_SHARE == req_uri.substr(0, URI_DB_ACCEPT_SHARE.size())) {
                handle_accept_share_request(req, pr);
        } else if (URI_DB_SEND_TO == req_uri.substr(0, URI_DB_SEND_TO.size())) {
                handle_send_to_peer_request(req, pr);
        } else if (URI_DB_MV_CACHE_TO_STORAGE ==
                        req_uri.substr(0, URI_DB_MV_CACHE_TO_STORAGE.size())) {
                ims->handle_move_cache_to_storage_request(&pr);
        } else if (URI_DB_REGISTER_LISTENER ==
                        req_uri.substr(0, URI_DB_REGISTER_LISTENER.size())) {
                handle_register_on_update(req, pr);
        } else if (URI_DB_UNREGISTER_LISTENER ==
                        req_uri.substr(0, URI_DB_UNREGISTER_LISTENER.size())) {
                handle_unregister_on_update(req, pr);
        } else if (URI_DB_DELETE ==
                        req_uri.substr(0, URI_DB_DELETE.size())) {
                handle_delete_request(req, pr);
        } else if (URI_DB_DUMP_USER_INFO ==
                        req_uri.substr(0, URI_DB_DUMP_USER_INFO.size())) {
                handle_dump_user_info(pr);
        } else {
                throw Poco::Exception("Unknown db request");
        }
        wait_for_response(fu, res);
        METAX_INFO("Response to database request sent to  "
                        + req.clientAddress().toString());
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (Poco::Exception& e) {
        METAX_ERROR(e.displayText());
        send_error_response(&res, e.displayText());
} catch (std::exception& e) {
        METAX_ERROR(e.what());
        send_error_response(&res, e.what());
} catch (...) {
        METAX_ERROR("Bad Request");
        send_error_response(&res, "Bad Request");
}

void leviathan::metax_web_api::db_request_handler::
handle_get_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        if (f.empty() || ! f.has("id")) {
                throw Poco::Exception("No UUID specified in get request");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        if (! ims->is_get_allowed(req)) {
                throw Poco::Exception("With the current configuration it "
                        "is not allowed to get data from non-localhost.");
        }
        std::pair<int64_t, int64_t> range = std::make_pair(-1, -1);
        if (req.has("Range")) {
                range = parse_range(req.get("Range"));
        }
        bool cache = true;
        if (f.has("cache") && "0" == f.get("cache")) {
                cache = false;
        }
        METAX_NOTICE("Received GET request for " + f.get("id") + " uuid.");
        ims->handle_get_request(&pr, f.get("id"),
                        range, cache, (long unsigned int)this);
}

void leviathan::metax_web_api::db_request_handler::
handle_copy_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        if (f.empty() || ! f.has("id")) {
                throw Poco::Exception("No UUID specified in /db/copy request");
        }
        Poco::Int64 e;
        parse_expire_date(f, e);
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        METAX_NOTICE("Received COPY request for " + f.get("id") + " uuid.");
        ims->handle_copy_request(&pr, f.get("id"), e, (long unsigned int)this);
}

void leviathan::metax_web_api::db_request_handler::
handle_save_request(Poco::Net::HTTPServerRequest& req,
                const std::string& req_uri,
                std::promise<web_api_adapter::ims_response>& pr)
{
        METAX_NOTICE("Received SAVE request.");
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        if (! ims->is_save_allowed(req)) {
                throw Poco::Exception("With the current configuration it "
                        "is not allowed to save/update data in non-localhost.");
        }
        URI u = get_request_type(req_uri);
        std::string c = "";
        if (req.has("Metax-Content-Type")) {
                c = req.get("Metax-Content-Type");
        }
        std::string s;
        Poco::UUID uuid = Poco::UUID::null();
        metax::ims_kernel_package::save_option l;
        Poco::Int64 e = DEFAULT_EXPIRE_DATE;
        bool enc = handle_web_save_request(req, u, s, uuid, l, e);
        if (save_stream == u) {
                if ("" != s) {
                        process_save_stream_request(pr, s, c, uuid, enc,
                                        (long unsigned int)this);
                } else {
                        throw Poco::Exception("Stream URL is not specified.");
                }
        } else if (save_node == u || ("" != s && Poco::File(s).exists()
                                && Poco::File(s).isFile())) {
                ims->handle_post_request(&pr, s, c, u, uuid, enc, l, e);
        } else {
                throw Poco::Exception("Invalid file path.");
        }
}

void leviathan::metax_web_api::db_request_handler::
process_save_stream_request(std::promise<web_api_adapter::ims_response>& pr,
                const std::string& s, const std::string& c,
                const Poco::UUID& uuid, bool enc, unsigned long client_id)
{
        Poco::Net::SocketAddress sa(s);
        m_livestream_socket.connect(sa);
        m_livestream_socket.setReceiveTimeout(40 * Poco::Timespan::SECONDS);
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_save_stream_request(&pr, s, c, uuid, enc, client_id);
}

void leviathan::metax_web_api::db_request_handler::
handle_accept_share_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        METAX_NOTICE("Received accept share request");
        Poco::Net::HTMLForm f(req, req.stream());
        if (! f.has("id") || ! f.has("key") || ! f.has("iv")) {
                std::string m = "Invalid accept_share request";
                throw Poco::Exception(m);
        }
        Poco::UUID u(f.get("id"));
        std::string k = f.get("key");
        std::string i = f.get("iv");
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_accept_share_request(&pr, u, k, i);
}

void leviathan::metax_web_api::db_request_handler::
handle_share_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        METAX_NOTICE("Received share request");
        Poco::UUID u = Poco::UUID::null();
        Poco::Net::HTMLForm f(req, req.stream());
        if (f.has("id")) {
                u.parse(f.get("id"));
        } else {
                std::string m = "File for sharing is not specified "
                        "for the request with ";
                m += f.has("username") ? f.get("username") : "public key";
                throw Poco::Exception(m);
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        std::string user = "";
        bool need_peer_key = true;
        if (f.has("key")) {
                user = f.get("key");
                if ("" == user) {
                        throw Poco::Exception("the key value is not valid");
                }
                platform::utils::trim(user);
                need_peer_key = false;
        } else if (f.has("username")) {
                user = f.get("username");
                need_peer_key = true;
        } else {
                throw Poco::Exception("Username or key is not specified");
        }
        ims->handle_share_request(&pr, user, u, need_peer_key);
}

void leviathan::metax_web_api::db_request_handler::
handle_send_to_peer_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        METAX_NOTICE("Received sendto request");
        std::string transferable_data = "";
        std::string user = "";
        Poco::Net::HTMLForm f(req, req.stream());
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        bool need_peer_key = true;
        if (f.has("key")) {
                user = f.get("key");
                if ("" == user) {
                        throw Poco::Exception("the key value is not valid");
                }
                platform::utils::trim(user);
                need_peer_key = false;
        } else if (f.has("username")) {
                user = f.get("username");
        } else {
                throw Poco::Exception("Username or key is not specified");
        }
        if (f.empty() || ! f.has("data") || 0 == f.get("data").size()) {
                std::string m = "Data for sending is not"
                        " specified for the request with ";
                if(f.has("username")) {
                        user = f.get("username");
                        m += user;
                } else {
                        m += "public key";
                }
                throw Poco::Exception(m);
        }
        transferable_data = f.get("data");
        ims->handle_send_to_peer_request(&pr, user, need_peer_key, transferable_data);
}

void leviathan::metax_web_api::db_request_handler::
handle_register_on_update(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        METAX_NOTICE("Received register on update request");
        if (f.empty() || ! f.has("id")) {
                throw Poco::Exception("No UUID specified in register_listener request");
        }
        Poco::UUID u(f.get("id"));
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_register_on_update_request(&pr, u);
}

std::string leviathan::metax_web_api::db_request_handler::
name() const
{
        return "metax_web_api.db_request_handler";
}

void leviathan::metax_web_api::db_request_handler::
handle_delete_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        METAX_NOTICE("Received delete request");
        if (f.empty() || ! f.has("id")) {
                throw Poco::Exception("No UUID specified in delete request");
        }
        bool keep_chunks = false;
        if (f.has("keep_chunks")) {
                keep_chunks = ("1" == f.get("keep_chunks"));
        }
        Poco::UUID u(f.get("id"));
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_delete_request(&pr, u, keep_chunks);
}

void leviathan::metax_web_api::db_request_handler::
handle_dump_user_info(std::promise<web_api_adapter::ims_response>& pr)
{
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_dump_user_info(&pr);
}

void leviathan::metax_web_api::db_request_handler::
handle_recording_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr,
                const std::string& req_uri)
{
        METAX_NOTICE("Received recording request");
        if (URI_DB_RECORDING_START == req_uri.substr(URI_DB_RECORDING.size(),
                                        URI_DB_RECORDING_START.size())) {
                handle_recording_start_request(req, pr);
        } else if (URI_DB_RECORDING_STOP ==
                        req_uri.substr( URI_DB_RECORDING.size(),
                                        URI_DB_RECORDING_STOP.size())) {
                handle_recording_stop_request(req, pr);
        } else {
                throw Poco::Exception(
                        "Recording should be either start or stop");
        }
}

void leviathan::metax_web_api::db_request_handler::
handle_recording_start_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        if (f.empty() || ! f.has("livestream")) {
                throw Poco::Exception("No livestream UUID "
                                "specified in start recording request");
        }
        Poco::UUID lu(f.get("livestream"));
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_recording_start_request(&pr, lu);
}

void leviathan::metax_web_api::db_request_handler::
handle_recording_stop_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        if (f.empty() || ! f.has("livestream") || ! f.has("recording")) {
                throw Poco::Exception("No livestream or recording UUID "
                                        "specified in stop recording request");
        }
        Poco::UUID lu(f.get("livestream"));
        Poco::UUID ru(f.get("recording"));
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_recording_stop_request(&pr, lu, ru);
}

void leviathan::metax_web_api::db_request_handler::
handle_unregister_on_update(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        METAX_NOTICE("Received unregister listener request");
        if (f.empty() || ! f.has("id")) {
                throw Poco::Exception("No UUID specified in unregister_listener request");
        }
        Poco::UUID u(f.get("id"));
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        ims->handle_unregister_on_update_request(&pr, u);
}

void leviathan::metax_web_api::db_request_handler::
set_response_headers(Poco::Net::HTTPServerResponse* r)
{
        r->set("Cache-Control", "no-cache, no-store, must-revalidate");
        r->set("Access-Control-Allow-Origin", "*");
        r->set("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
        r->set("Access-Control-Max-Age", "1000");
        r->set("Access-Control-Allow-Headers",
                                "Content-Type, Metax-Content-Type");
}

void leviathan::metax_web_api::db_request_handler::
send_error_response(Poco::Net::HTTPServerResponse* r, const std::string& s)
{
        std::string j = R"({"error": ")" + s + R"("})";
        r->setStatus(Poco::Net::HTTPServerResponse::HTTP_BAD_REQUEST);
        r->sendBuffer(j.c_str(), j.size());
}

void leviathan::metax_web_api::db_request_handler::
wait_for_response(std::future<web_api_adapter::ims_response>& fu,
                Poco::Net::HTTPServerResponse& res)
{
        web_api_adapter::ims_response ir = fu.get();
        res.setStatus(ir.st);
        switch (ir.kind) {
                case web_api_adapter::media_streaming: {
                        start_media_streaming(res, ir);
                        break;
                } case web_api_adapter::file: {
                        send_file(res, ir);
                        break;
                } case web_api_adapter::content: {
                        if ("" != ir.content_type) {
                                res.set("Content-Type", ir.content_type);
                        }
                        res.sendBuffer(ir.info.c_str(), ir.info.size());
                        break;
                } case web_api_adapter::livestream_start: {
                        read_livestream_from_socket(ir, res);
                        break;
                } case web_api_adapter::livestream_content: {
                        start_sending_livestream(ir, res);
                        break;
                } default: {
                        METAX_ERROR("Unexpected response from web api adapter");
                }
        }
}

void leviathan::metax_web_api::db_request_handler::
send_file(Poco::Net::HTTPServerResponse& res,
                web_api_adapter::ims_response& ir)
{
        Poco::File fp(ir.response_file_path->path());
        if (! fp.exists()) {
                throw Poco::Exception("file " + ir.response_file_path->path()
                                + " not found");
        }
        res.sendFile(ir.response_file_path->path(), ir.content_type);
}

void leviathan::metax_web_api::db_request_handler::
start_media_streaming(Poco::Net::HTTPServerResponse& res,
                web_api_adapter::ims_response& ir)
try {
        set_streaming_headers(res, ir);
        std::ostream& ostr = res.send();
        METAX_DEBUG("Start streaming audio/video file");
        for (auto& fu : *(ir.stream_futures)) {
                web_api_adapter::ims_response r = fu.get();
                Poco::File fp(r.response_file_path->path());
                if (! fp.exists()) {
                        METAX_ERROR("Getting media file is interrupted: file "
                                        + r.response_file_path->path() +
                                        " not found");
                        break;
                }
                std::ifstream ifs(r.response_file_path->path(),
                                std::ios::binary);
                Poco::StreamCopier::copyStream(ifs, ostr);
                ostr << std::flush;
        }
        METAX_DEBUG("Streaming audio/video file finished");
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        // This sleep helps to improve performance in case of seek. When the
        // user seeks the video new request is sent by client and the old one
        // is cleaned. The sleep helps the new request to reach Kernel earlier
        // than clean request, so that the new request can reuse already saved
        // chunks from the old request.
        Poco::Thread::sleep(1000);
        // TODO - improve error handling when client is gone in the middle.
        ims->clean_streaming_request((unsigned long int)this);
} catch (...) {
        METAX_ERROR("Getting media file is interrupted");
}

void leviathan::metax_web_api::db_request_handler::
read_from_socket(std::future<web_api_adapter::ims_response>& e,
                web_api_adapter* ims)
{
        poco_assert(nullptr != ims);
        const int ssize = 30000;
        char buf[ssize];
        // ignore first portion
        m_livestream_socket.receiveBytes(buf, ssize);
        int s = 0;
        while (true) {
                auto status = e.wait_for(std::chrono::milliseconds(0));
                if (status == std::future_status::ready) {
                        web_api_adapter::ims_response er = e.get();
                        break;
                }
                int rs = m_livestream_socket.receiveBytes(buf + s, ssize - s);
                if (0 == rs) {
                        if (s != 0) {
                                ims->handle_livestream_content(buf, s,
                                                        (unsigned long)this);
                        }
                        ims->cancel_livestream((unsigned long)this);
                        break;
                }
                s += rs;
                poco_assert(ssize >= s);
                if (ssize == s) {
                        ims->handle_livestream_content(buf, s,
                                                        (unsigned long)this);
                        s = 0;
                }
        }
}

void leviathan::metax_web_api::db_request_handler::
read_livestream_from_socket(web_api_adapter::ims_response& ir,
                Poco::Net::HTTPServerResponse& res)
{
        poco_assert(nullptr != ir.stream_futures);
        poco_assert(1 == ir.stream_futures->size());
        std::ostream& out = res.send();
        std::future<web_api_adapter::ims_response>& e = (*ir.stream_futures)[0];
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        try {
                read_from_socket(e, ims);
        } catch (const Poco::Exception& exc) {
                out << exc.displayText() << std::flush;
                ims->cancel_livestream((unsigned long)this);
                METAX_ERROR(exc.displayText());
        } catch (...) {
                ims->cancel_livestream((unsigned long)this);
                METAX_ERROR("Fail to read stream from socket");
        }
}

void leviathan::metax_web_api::db_request_handler::
start_sending_livestream(web_api_adapter::ims_response& ir,
                Poco::Net::HTTPServerResponse& res)
try {
        std::ostream& ostr = res.send();
        ostr.write(ir.info.c_str(), (long)ir.info.size());
        ostr << std::flush;
        while (true) {
                poco_assert(nullptr != ir.stream_futures);
                poco_assert(1 == ir.stream_futures->size());
                auto& lvsfu = (*ir.stream_futures)[0];
                ir = lvsfu.get();
                if (Poco::Net::HTTPResponse::HTTP_GONE == ir.st) {
                        break;
                }
                ostr.write(ir.info.c_str(), (long)ir.info.size());
                ostr << std::flush;
                if (! ostr.good()) {
                        web_api_adapter* ims = web_api_adapter::get_instance();
                        poco_assert(nullptr != ims);
                        ims->cancel_livestream_get((unsigned long)this);
                        break;
                }
        }
} catch (...) {
        METAX_ERROR("Livestream ended");
}

void leviathan::metax_web_api::db_request_handler::
set_streaming_headers(Poco::Net::HTTPServerResponse& res,
                        const web_api_adapter::ims_response& ir)
{
        res.set("Content-Type", ir.content_type);
        res.set("Accept-Ranges", "bytes");
        if (0 <= ir.range.first) {
                poco_assert(-1 != ir.range.second);
                std::ostringstream s2;
                s2 << "bytes " << ir.range.first;
                s2 << '-' << ir.range.second << '/' << ir.content_length;
                res.set("Content-Range", s2.str());
                res.setStatus(
                        Poco::Net::HTTPServerResponse::HTTP_PARTIAL_CONTENT);
                res.setContentLength((std::streamsize)
                                (ir.range.second - ir.range.first + 1));
        } else {
                res.setContentLength((std::streamsize)(ir.content_length));
        }
}

std::pair<int64_t, int64_t> leviathan::metax_web_api::db_request_handler::
parse_range(const std::string& range)
{
        std::string::const_iterator it = range.begin();
        std::string::const_iterator end = range.end();
        int64_t start = -1;
        int64_t length = -1;
        while (it != end && Poco::Ascii::isSpace(*it)) ++it;
        std::string unit;
        while (it != end && *it != '=') unit += *it++;
        if (unit == "bytes" && it != end) {
                ++it;
                if (it != end && *it == '-') {
                        ++it;
                        length = 0;
                        while (it != end && Poco::Ascii::isDigit(*it)) {
                                length *= 10;
                                length += *it - '0';
                                ++it;
                        }
                        if (0 != length) {
                                return std::make_pair(start, length);
                        }
                } else if (it != end && Poco::Ascii::isDigit(*it)) {
                        start = 0;
                        while (it != end && Poco::Ascii::isDigit(*it)) {
                                start *= 10;
                                start += *it - '0';
                                ++it;
                        }
                        if (it != end && *it == '-') {
                                ++it;
                                if (it != end && *it != ',' && ! Poco::Ascii::isSpace(*it)) {
                                        length = 0;
                                        while (it != end && Poco::Ascii::isDigit(*it)) {
                                                length *= 10;
                                                length += *it - '0';
                                                ++it;
                                        }
                                        length = length - start + 1;
                                        if ((it != end && *it != ',' && ! Poco::Ascii::isSpace(*it))
                                                        || 0 >= length) {
                                                throw Poco::Exception("Invalid range specifier");
                                        }
                                }
                                return std::make_pair(start, length);
                        }
                }
        }
        throw Poco::Exception("Invalid range specifier");
}

const std::string& leviathan::metax_web_api::upload_handler::
get_upload_file_path()
{
        return m_upload_path;
}

void leviathan::metax_web_api::upload_handler::
handlePart(const Poco::Net::MessageHeader& header, std::istream& s)
{
        //TODO - add error handling
        if (header.has("Content-Disposition")) {
                std::string disp;
                Poco::Net::NameValueCollection params;
                Poco::Net::MessageHeader::splitParameters(
                                header["Content-Disposition"], disp, params);
                Poco::TemporaryFile tf(leviathan::platform::utils::tmp_dir);
                tf.keepUntilExit();
                std::ofstream o(tf.path().c_str(), std::ios::binary);
                o << s.rdbuf();
                o.close();
                m_upload_path = tf.path();
        }
}

leviathan::metax_web_api::upload_handler::
upload_handler()
        : m_upload_path("")
{}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

