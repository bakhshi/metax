/**
 * @file src/metax_web_api/websocket_request_handler.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::websocket_request_handler,
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

#ifndef LEVIATHAN_METAX_WEB_API_WEBSOCKET_REQUEST_HANDLER_HPP
#define LEVIATHAN_METAX_WEB_API_WEBSOCKET_REQUEST_HANDLER_HPP


// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include "Poco/Net/WebSocket.h"
#include <Poco/Logger.h>

// Headers from standard libraries
#include <memory>

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class websocket_request_handler;
        }
}

/**
 * @brief Handles web socket connection from web client
 * 
 */
class leviathan::metax_web_api::websocket_request_handler
        : public Poco::Net::HTTPRequestHandler
{

        /// @name Public members
public:
        std::string peer_address;

        /// @name Public interface
public:
        /**
         * @brief Processes newly connected websocket.
         */
        void handleRequest(Poco::Net::HTTPServerRequest& request, 
                        Poco::Net::HTTPServerResponse& response);

        std::string name() const;

        bool send(const char* m, int s);

        bool send(const std::string& m);

        void close();

        /// @name Special member functions
public:
        /// Default constructor
        websocket_request_handler()
                : m_logger(Poco::Logger::
                                get("metax_web_api.websocket_request_handler"))
        {
        }

        /// Destructor
        virtual ~websocket_request_handler() {};

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        websocket_request_handler(const websocket_request_handler&) = delete;

        /// This class is not assignable
        websocket_request_handler& operator=( const websocket_request_handler&)
                                                                = delete;
        /// @name Private data
private:
        std::unique_ptr<Poco::Net::WebSocket> m_websocket ;
        Poco::Logger& m_logger;

}; // class leviathan::metax_web_api::websocket_request_handler

#endif // LEVIATHAN_METAX_WEB_API_WEBSOCKET_REQUEST_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

