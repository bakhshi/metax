/**
 * @file src/metax/socket_reactor.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::socket_reactor.
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

#ifndef LEVIATHAN_METAX_SOCKET_REACTOR_HPP
#define LEVIATHAN_METAX_SOCKET_REACTOR_HPP


// Headers from this project

// Headers from other projects
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/Net/SocketReactor.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct socket_reactor;
                struct link;
        }
}

/**
 * @brief Extends Poco::Net::SocketReactor to keep pointer to link layer.
 * Pointer to link is passed to link_peer objects when socket reactor
 * instantiates them upon receiving peer connections.
 * 
 */
struct leviathan::metax::socket_reactor
        : public Poco::Net::SocketReactor
{
        /// @name Public interface
public:
        link* get_link() const;

        /// @name Special member functions
public:
        /**
         * Constructor
         *
         * @param l - the link layer object
         */
        socket_reactor(link* l);

        /// Destructor
        virtual ~socket_reactor();

        /// @name Private data
private:
        link* m_link;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        socket_reactor(const socket_reactor&) = delete;

        /// This class is not assignable
        socket_reactor& operator=(const socket_reactor&) = delete;

}; // class leviathan::metax::socket_reactor

#endif // LEVIATHAN_METAX_SOCKET_REACTOR_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

