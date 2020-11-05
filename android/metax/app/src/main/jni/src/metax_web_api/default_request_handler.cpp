/**
 * @file src/metax_web_api/default_request_handler.cpp
 *
 * @brief Implementation of the class
 * @ref leviathan::metax_web_api::default_request_handler.
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
#include "default_request_handler.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include "Poco/Net/HTMLForm.h"
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

// Headers from standard C++ libraries
#include <fstream>

void leviathan::metax_web_api::default_request_handler::
handleRequest(Poco::Net::HTTPServerRequest& request,
        Poco::Net::HTTPServerResponse& response)
try {
        METAX_INFO("Received sitemap request from  "
                        + request.clientAddress().toString());
        std::string uri = request.getURI();
        std::string host = request.getHost();
        METAX_INFO("Requesting URL: " + host + uri);
        set_response_headers(response);
        sitemap s = m_sitemap->getObject(host);
        std::string du = "";
        Poco::URI::decode(uri, du);
        std::string p = s->getValue<std::string>(du);
        std::promise<web_api_adapter::ims_response> pr;
        std::future<web_api_adapter::ims_response> fu = pr.get_future();
        handle_get_request(request, p, pr);
        wait_for_response(fu, response);
        METAX_INFO("Response to sitemap request sent to  "
                        + request.clientAddress().toString());
} catch (not_allowed_exception& e) {
        std::string not_allowed = e.displayText();
        response.setStatus(Poco::Net::HTTPServerResponse::HTTP_FORBIDDEN);
        response.sendBuffer(not_allowed.c_str(), not_allowed.size());
        METAX_ERROR(not_allowed);
} catch (...) {
        std::string not_found = R"({"error":"404 Not Found"})";
        response.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
        response.sendBuffer(not_found.c_str(), not_found.size());
        METAX_ERROR("URL not found");
}

void leviathan::metax_web_api::default_request_handler::
handle_get_request(Poco::Net::HTTPServerRequest& req,
                const std::string& p,
                std::promise<web_api_adapter::ims_response>& pr)
{
        Poco::Net::HTMLForm f(req);
        if (p.empty()) {
                throw Poco::Exception("No UUID specified in get request");
        }
        web_api_adapter* ims = web_api_adapter::get_instance();
        poco_assert(nullptr != ims);
        if (! ims->is_get_allowed(req)) {
                throw not_allowed_exception("With the current configuration it "
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
        METAX_NOTICE("Received GET request for " + p + " uuid.");
        ims->handle_get_request(&pr, p,
                        range, cache, (long unsigned int)this);
}

void leviathan::metax_web_api::default_request_handler::
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
                        Poco::File fp(ir.response_file_path->path());
                        if (! fp.exists()) {
                                throw Poco::Exception(
                                        "file " + ir.response_file_path->path()
                                        + " not found");
                        }
                        res.sendFile(ir.response_file_path->path(), ir.content_type);
                        break;
                } case web_api_adapter::content: {
                        if ("" != ir.content_type) {
                                res.set("Content-Type", ir.content_type);
                        }
                        res.sendBuffer(ir.info.c_str(), ir.info.size());
                        break;
                } case web_api_adapter::livestream_content: {
                        start_sending_livestream(ir, res);
                        break;
                } default: {
                        METAX_ERROR("Unexpected response from web api adapter");
                }
        }
}

std::pair<int64_t, int64_t>
leviathan::metax_web_api::default_request_handler::
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

void leviathan::metax_web_api::default_request_handler::
start_media_streaming(Poco::Net::HTTPServerResponse& res,
                web_api_adapter::ims_response& ir)
{
        set_streaming_headers(res, ir);
        std::ostream& ostr = res.send();
        for (auto& fu : *(ir.stream_futures)) {
                web_api_adapter::ims_response r = fu.get();
                Poco::File fp(r.response_file_path->path());
                if (! fp.exists()) {
                        throw Poco::Exception("file " + r.response_file_path->path() + " not found");
                }
                std::ifstream ifs(r.response_file_path->path(), std::ios::binary);
                Poco::StreamCopier::copyStream(ifs, ostr);
                ostr << std::flush;
        }
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
}

void leviathan::metax_web_api::default_request_handler::
start_sending_livestream(web_api_adapter::ims_response& ir,
                Poco::Net::HTTPServerResponse& res)
{
        std::ostream& ostr = res.send();
        ostr.write(ir.info.c_str(), (long)ir.info.size());
        ostr << std::flush;
        while (true) {
                poco_assert(nullptr != ir.stream_futures);
                poco_assert(1 == ir.stream_futures->size());
                auto& lvsfu = (*ir.stream_futures)[0];
                ir = lvsfu.get();
                ostr.write(ir.info.c_str(), (long)ir.info.size());
                ostr << std::flush;
                if (Poco::Net::HTTPResponse::HTTP_GONE == ir.st) {
                        break;
                }
                if (! ostr.good()) {
                        web_api_adapter* ims = web_api_adapter::get_instance();
                        poco_assert(nullptr != ims);
                        ims->cancel_livestream_get((unsigned long)this);
                        break;
                }
        }
}

void leviathan::metax_web_api::default_request_handler::
set_response_headers(Poco::Net::HTTPServerResponse& res)
{
        res.set("Cache-Control", "no-cache, no-store, must-revalidate");
        res.set("Access-Control-Allow-Origin", "*");
        res.set("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
        res.set("Access-Control-Max-Age", "1000");
        res.set("Access-Control-Allow-Headers",
                                "Content-Type, Metax-Content-Type");
}

void leviathan::metax_web_api::default_request_handler::
set_streaming_headers(Poco::Net::HTTPServerResponse& res,
                        const web_api_adapter::ims_response& ir)
{
        res.set("Content-Type", ir.content_type);
        res.set("Accept-Ranges", "bytes");
        if (-1 != ir.range.first) {
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

std::string leviathan::metax_web_api::default_request_handler::
name() const
{
        return "metax_web_api.default_request_handler";
}

leviathan::metax_web_api::default_request_handler::
default_request_handler(sitemap sm)
        : m_logger(Poco::Logger::get("metax_web_api.default_request_handler"))
        , m_sitemap(sm)
{
}

leviathan::metax_web_api::default_request_handler::
~default_request_handler()
{
}

leviathan::metax_web_api::not_allowed_exception::
not_allowed_exception(const std::string& m)
        : Exception(m)
{
}


// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

