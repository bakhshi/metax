/**
 * @file src/metax_web_api/web_request_handler_factory.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::web_request_handler_factory.
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
#include "web_request_handler_factory.hpp"
#include "db_request_handler.hpp"
#include "default_request_handler.hpp"
#include "email_sender.hpp"
#include "websocket_request_handler.hpp"
#include "configuration_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/HTTPServerRequest.h>

// Headers from standard libraries

Poco::Net::HTTPRequestHandler*
leviathan::metax_web_api::web_request_handler_factory::
createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
        std::string uri = request.getURI();
        std::string d = "/db";
        std::string e = "/sendemail";
        std::string c = "/config";
        if ( d == uri.substr(0, d.size()) ) {
                return new db_request_handler;
        } else if (e == uri.substr(0, e.size())) {
                return new email_sender;
        } else if (c == uri.substr(0, c.size())) {
                return new configuration_handler;
        } else if(request.find("Upgrade") != request.end() &&
                        Poco::icompare(request["Upgrade"], "websocket") == 0) {
                        return new websocket_request_handler;
        } else {
                return new default_request_handler(m_sitemap);
	 }
}

leviathan::metax_web_api::web_request_handler_factory::
web_request_handler_factory(sitemap sm)
        : m_sitemap(sm)
{}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

