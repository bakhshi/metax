/**
 * @file src/metax_web_api/websocket_request_handler.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::websocket_request_handler.
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
#include "websocket_request_handler.hpp"
#include "web_api_adapter.hpp"

// Headers from other projects

// Headers from third party libraries
#include "Poco/Timespan.h"

// Headers from standard libraries

void leviathan::metax_web_api::websocket_request_handler::
handleRequest(Poco::Net::HTTPServerRequest& req,
                Poco::Net::HTTPServerResponse& res)
try {
        peer_address = req.clientAddress().toString();
        METAX_INFO("Received websocket request from  " + peer_address);
        web_api_adapter* waa = web_api_adapter::get_instance();
        poco_assert(nullptr != waa);
        m_websocket = std::unique_ptr<Poco::Net::WebSocket>(
                                new Poco::Net::WebSocket(req, res));
        waa->add_websocket(this);
        METAX_INFO("Connection established");
        const int n = 10000;
        char buf[n];
        try {
                while (true) {
                        bool b = m_websocket->poll(Poco::Timespan(0, 0, 1, 0, 0),
                                        Poco::Net::WebSocket::SELECT_READ);
                        if (b && 0 == m_websocket->receiveBytes(buf, n)) {
                                waa->remove_websocket(this);
                                break;
                        }
                }
        } catch (...) {
                METAX_INFO("Websocket exception: - " + peer_address);
                waa->remove_websocket(this);
                throw;
        }
} catch (const Poco::AssertionViolationException& e) {
        METAX_INFO("Websocket error: " + e.displayText() + ": " + peer_address);
        abort();
} catch (Poco::Exception& e) {
        METAX_INFO("Websocket error: " + e.displayText() +
                                " - " + req.clientAddress().toString());
        std::string j = R"({"error": ")" + e.displayText() + R"("})";
        res.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
        res.sendBuffer(j.c_str(), j.size());
} catch (...) {
        METAX_INFO("Unhandled websocket exception: " + peer_address);
        std::string j = R"({"error": "Unhandled exception"})";
        res.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
        res.sendBuffer(j.c_str(), j.size());
}

std::string leviathan::metax_web_api::websocket_request_handler::
name() const
{
        return "metax_web_api.websocket_request_handler";
}

bool leviathan::metax_web_api::websocket_request_handler::
send(const char* m, int s)
{
        bool b = m_websocket->poll(Poco::Timespan(0, 0, 0, 0, 0),
                        Poco::Net::WebSocket::SELECT_WRITE);
        if (b) {
                m_websocket->sendFrame(m, (int)s);
        }
        return b;
}

bool leviathan::metax_web_api::websocket_request_handler::
send(const std::string& m)
{
        return send(m.c_str(), (int)m.size());
}

void leviathan::metax_web_api::websocket_request_handler::
close()
{
        m_websocket->close();
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

