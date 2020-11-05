/**
 * @file src/metax_web_api/refuse_handler.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::refuse_handler.
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

#ifndef LEVIATHAN_METAX_WEB_API_REFUSE_HANDLER_HPP
#define LEVIATHAN_METAX_WEB_API_REFUSE_HANDLER_HPP

// Headers from this project

// Headers from other project

// Headers from third party libraries
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Logger.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class refuse_handler;
        }
}

/**
 * @brief Sends error message as response informing that invalid security token
 * is provided
 */
class leviathan::metax_web_api::refuse_handler
        : public Poco::Net::HTTPRequestHandler
{
        /// @name Public interface
public:
        /**
         * @brief Used for sending error message as an http response.
         *
         */
        void handleRequest(Poco::Net::HTTPServerRequest&,
                        Poco::Net::HTTPServerResponse&) override;

        /// @name Helper Functions
private:
        void set_response_headers(Poco::Net::HTTPServerResponse&);

        std::string name() const;

        /// @name Member variables
private:
        Poco::Logger& m_logger;

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        refuse_handler();

        /// Destructor
        ~refuse_handler();

        /// @name Undefined, hidden special member functions
private:
        /// This class is not copy-constructible
        refuse_handler(const refuse_handler&);

        /// This class is not assignable
        refuse_handler& operator=(const refuse_handler&);

}; // class leviathan::metax_web_api::refuse_handler

#endif // LEVIATHAN_METAX_WEB_API_REFUSE_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

