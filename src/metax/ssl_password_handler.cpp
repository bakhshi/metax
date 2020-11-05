/**
 * @file
 *
 * @brief Implementation of the class @ref
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

// Headers from this project
#include "ssl_password_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/PrivateKeyFactory.h>

// Headers from standard libraries

//POCO_REGISTER_KEYFACTORY(leviathan::metax, leviathan::metax::ssl_password_handler)

const std::string leviathan::metax::ssl_password_handler::
METAX_SSL_PASSPHRASE = "leviathan";

void leviathan::metax::ssl_password_handler::
onPrivateKeyRequested(const void * pSender, std::string & privateKey)
{
        privateKey = METAX_SSL_PASSPHRASE;
}

leviathan::metax::ssl_password_handler::
ssl_password_handler(bool on_server_side)
        : PrivateKeyPassphraseHandler(on_server_side)
{}

leviathan::metax::ssl_password_handler::
~ssl_password_handler()
{}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

