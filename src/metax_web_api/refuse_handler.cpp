/**
 * @file src/metax_web_api/refuse_handler.cpp
 *
 * @brief Implementation of the class
 * @ref leviathan::metax_web_api::refuse_handler.
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
#include "refuse_handler.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

// Headers from standard C++ libraries

void leviathan::metax_web_api::refuse_handler::
handleRequest(Poco::Net::HTTPServerRequest& request,
        Poco::Net::HTTPServerResponse& response)
{
        METAX_ERROR("Received request with invalid security token");
        std::string res = R"({"error":")";
        res += R"("invlid or no security token provided to http(s) request"})";
        set_response_headers(response);
        response.setStatus(Poco::Net::HTTPServerResponse::HTTP_FORBIDDEN);
        response.sendBuffer(res.c_str(), res.size());
}

void leviathan::metax_web_api::refuse_handler::
set_response_headers(Poco::Net::HTTPServerResponse& res)
{
        res.set("Cache-Control", "no-cache, no-store, must-revalidate");
        res.set("Access-Control-Allow-Origin", "*");
        res.set("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
        res.set("Access-Control-Max-Age", "1000");
        res.set("Access-Control-Allow-Headers",
                                "Content-Type, Metax-Content-Type");
}

std::string leviathan::metax_web_api::refuse_handler::
name() const
{
        return "metax_web_api.refuse_handler";
}

leviathan::metax_web_api::refuse_handler::
refuse_handler()
        : m_logger(Poco::Logger::get("metax_web_api.refuse_handler"))
{
}

leviathan::metax_web_api::refuse_handler::
~refuse_handler()
{
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

