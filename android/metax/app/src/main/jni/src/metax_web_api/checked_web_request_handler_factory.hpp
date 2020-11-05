/**
 * @file src/metax_web_api/checked_web_request_handler_factory.hpp
 *
 * @brief Definition of the classes @ref
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

#ifndef LEVIATHAN_METAX_WEB_API_CHECKED_WEB_REQUEST_HANDLER_FACTORY_HPP
#define LEVIATHAN_METAX_WEB_API_CHECKED_WEB_REQUEST_HANDLER_FACTORY_HPP


// Headers from this project
#include "web_request_handler_factory.hpp"

// Headers from other projects

// Headers from third party libraries

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class checked_web_request_handler_factory;
        }
}

/**
 * @brief Checks whether the security token provided by the http(s) request is
 * valid, then creates new request handler instance, otherwise error response is
 * sent.
 * 
 */
class leviathan::metax_web_api::checked_web_request_handler_factory
        : public leviathan::metax_web_api::web_request_handler_factory
{
        /// @name Public interface
public:
        /**
         * @brief Checks whether the security token provided by the http(s)
         * request is valid, then creates http request handler based on the
         * first part of the request URL.
         */
        Poco::Net::HTTPRequestHandler* createRequestHandler(
                const Poco::Net::HTTPServerRequest& request) override;

        /// @name Special member functions
public:
        /// Default Constructor
        checked_web_request_handler_factory(sitemap sm, const std::string& token);

        /// Destructor
        virtual ~checked_web_request_handler_factory() {};

        /// Private members
private:
        std::string m_security_token;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        checked_web_request_handler_factory(
                        const checked_web_request_handler_factory&) = delete;

        /// This class is not assignable
        checked_web_request_handler_factory& operator=(
                        const checked_web_request_handler_factory&) = delete;

}; // class leviathan::metax_web_api::checked_web_request_handler_factory


#endif // LEVIATHAN_METAX_WEB_API_CHECKED_WEB_REQUEST_HANDLER_FACTORY_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

