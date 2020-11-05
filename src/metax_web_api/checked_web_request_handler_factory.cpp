/**
 * @file src/metax_web_api/checked_web_request_handler_factory.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::checked_web_request_handler_factory.
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
#include "checked_web_request_handler_factory.hpp"
#include "refuse_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTMLForm.h>

// Headers from standard libraries

Poco::Net::HTTPRequestHandler*
leviathan::metax_web_api::checked_web_request_handler_factory::
createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
        if (request.hasCredentials()) {
                std::string s = "";
                std::string i = "";
                request.getCredentials(s, i);
                if ("Metax-Auth" == s && i == m_security_token) {
                        return web_request_handler_factory::createRequestHandler(request);
                }
        }
        Poco::Net::HTMLForm f(request);
        if (! f.empty() && f.has("token") && f.get("token") == m_security_token) {
                return web_request_handler_factory::createRequestHandler(request);
        }
        return new refuse_handler;
}

leviathan::metax_web_api::checked_web_request_handler_factory::
checked_web_request_handler_factory(sitemap sm, const std::string& token)
        : web_request_handler_factory(sm)
        , m_security_token(token)
{
        poco_assert(! m_security_token.empty());
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

