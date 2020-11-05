/**
 * @file src/metax_web_api/configuration_handler.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::configuration_handler.
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

#ifndef LEVIATHAN_METAX_WEB_API_CONFIGURATION_HANDLER_HPP
#define LEVIATHAN_METAX_WEB_API_CONFIGURATION_HANDLER_HPP

// Headers from this project
#include "web_api_adapter.hpp"

// Headers from other project

// Headers from third party libraries
#include <Poco/Net/HTTPRequestHandler.h>

// Headers from standard libraries
#include <future>

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class configuration_handler;
        }
}

/**
 * @brief Handles http requests from web client related to Metax configurations.
 */
class leviathan::metax_web_api::configuration_handler
        : public Poco::Net::HTTPRequestHandler
{
        /// Const members associated with web request url
private:
        static const std::string URI_CONFIG;
        static const std::string URI_CONFIG_ADD_FRIEND;
        static const std::string URI_CONFIG_GET_FRIEND_LIST;
        static const std::string URI_CONFIG_ADD_PEER;
        static const std::string URI_CONFIG_REMOVE_PEER;
        static const std::string URI_CONFIG_GET_PEER_LIST;
        static const std::string URI_CONFIG_GET_USER_PUBLIC_KEY;
        static const std::string URI_CONFIG_GET_USER_KEYS;
        static const std::string URI_CONFIG_GET_ONLINE_PEERS;
        static const std::string URI_CONFIG_GET_METAX_INFO;
        static const std::string URI_CONFIG_SET_METAX_INFO;
        static const std::string URI_CONFIG_RECONNECT_TO_PEERS;
        static const std::string URI_CONFIG_START_PAIRING;
        static const std::string URI_CONFIG_CANCEL_PAIRING;
        static const std::string URI_CONFIG_GET_PAIRING_PEERS;
        static const std::string URI_CONFIG_CONNECT_SERVER;
        static const std::string URI_CONFIG_REGENERATE_KEYS;
        Poco::Logger& m_logger;

        /// Public interface
private:
        void handleRequest(Poco::Net::HTTPServerRequest&,
                        Poco::Net::HTTPServerResponse&);

        /// Member variables
private:

        /// Helper functions
private:
        void set_response_header(Poco::Net::HTTPServerResponse*);
        void wait_for_response(std::future<web_api_adapter::ims_response>& fu,
                Poco::Net::HTTPServerResponse& res);
        void handle_add_trusted_peer_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_remove_trusted_peer_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_add_friend(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_set_metax_info(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_get_user_keys_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_start_pairing(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_request_keys(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        void handle_regenerate_user_keys(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);
        std::string name() const;

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        //configuration_handler(const std::string&);
        configuration_handler();

        /// Destructor
        ~configuration_handler();

        /// @name Undefined, hidden special member functions
private:
        /// This class is not copy-constructible
        configuration_handler(const configuration_handler&);

        /// This class is not assignable
        configuration_handler& operator=(const configuration_handler&);

}; // class leviathan::metax_web_api::configuration_handler

#endif // LEVIATHAN_METAX_WEB_API_CONFIGURATION_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

