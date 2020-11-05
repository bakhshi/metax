/**
 * @file src/metax_web_api/default_request_handler.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::default_request_handler.
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

#ifndef LEVIATHAN_METAX_WEB_API_DEFAULT_REQUEST_HANDLER_HPP
#define LEVIATHAN_METAX_WEB_API_DEFAULT_REQUEST_HANDLER_HPP

// Headers from this project
#include "web_api_adapter.hpp"

// Headers from other project

// Headers from third party libraries
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Logger.h>

// Headers from standard libraries
#include <future>

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class default_request_handler;
                struct not_allowed_exception;
        }
}

/**
 * @brief Handles requests that do not match to any other request type, e.g. db
 * or config. Tries to perform get request based on sitemap information.
 */
class leviathan::metax_web_api::default_request_handler
        : public Poco::Net::HTTPRequestHandler
{
        /// Type definitions
public:
        using sitemap = Poco::JSON::Object::Ptr;

        /// @name Public interface
public:
        /**
         * @brief Processes http requests for which no dedicated handler exists.
         * Tries to perform get request based on sitemap.
         *
         */
        void handleRequest(Poco::Net::HTTPServerRequest&,
                        Poco::Net::HTTPServerResponse&) override;

        /// @name Member variables
private:
        Poco::Logger& m_logger;

        sitemap m_sitemap;

        //std::string m_host;

        /// @name Helper functions
private:
        void handle_get_request(Poco::Net::HTTPServerRequest& req,
                        const std::string& p,
                        std::promise<web_api_adapter::ims_response>& pr);

        std::string name() const;

        void wait_for_response(std::future<web_api_adapter::ims_response>& fu,
                                Poco::Net::HTTPServerResponse& res);

        std::pair<int64_t, int64_t> parse_range(const std::string& range);

        void set_response_headers(Poco::Net::HTTPServerResponse& res);

        void start_media_streaming(Poco::Net::HTTPServerResponse& res,
                                        web_api_adapter::ims_response& ir);

        void start_sending_livestream(web_api_adapter::ims_response& ir,
                                        Poco::Net::HTTPServerResponse& res);

        void set_streaming_headers(Poco::Net::HTTPServerResponse& res,
                                const web_api_adapter::ims_response& ir);

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        default_request_handler(sitemap);
        //default_request_handler(const std::string&);

        /// Destructor
        ~default_request_handler();

        /// @name Undefined, hidden special member functions
private:
        /// This class is not copy-constructible
        default_request_handler(const default_request_handler&);

        /// This class is not assignable
        default_request_handler& operator=(const default_request_handler&);

}; // class leviathan::metax_web_api::default_request_handler

struct leviathan::metax_web_api::not_allowed_exception
        : public Poco::Exception
{
        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        not_allowed_exception(const std::string& m);

}; // class leviathan::metax_web_api::not_allowed_exception

#endif // LEVIATHAN_METAX_WEB_API_DEFAULT_REQUEST_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

