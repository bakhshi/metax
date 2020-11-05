/**
 * @file src/metax/ssl_password_handler.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::ssl_password_handler.
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

#ifndef LEVIATHAN_METAX_SSL_PASSWORD_HANDLER_HPP
#define LEVIATHAN_METAX_SSL_PASSWORD_HANDLER_HPP


// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/PrivateKeyPassphraseHandler.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct ssl_password_handler;
        }
}

/**
 * @brief Manages password of Metax RSA keys
 * 
 */
struct leviathan::metax::ssl_password_handler
        : Poco::Net::PrivateKeyPassphraseHandler
{
        /// @name Type definitions
public:
        virtual void onPrivateKeyRequested(const void* pSender,
                                                std::string& privateKey);

        /// @name Special member functions
public:
        /// Constructor
        ssl_password_handler(bool on_server_side);

        /// Destructor
        virtual ~ssl_password_handler();

        /// @name Static members
public:
        static const std::string METAX_SSL_PASSPHRASE;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        ssl_password_handler(const ssl_password_handler&) = delete;

        /// This class is not assignable
        ssl_password_handler& operator=(const ssl_password_handler&) = delete;

}; // class leviathan::metax::ssl_password_handler

#endif // LEVIATHAN_METAX_SSL_PASSWORD_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

