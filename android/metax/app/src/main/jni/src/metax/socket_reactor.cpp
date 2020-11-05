/**
 * @file src/metax/socket_reactor.cpp
 *
 * @brief Implementation of the class @ref
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

// Headers from this project
#include "socket_reactor.hpp"
#include "link.hpp"

// Headers from other projects
//#include <net/socket.hpp>

// Headers from third party libraries

// Headers from standard libraries

leviathan::metax::link* leviathan::metax::socket_reactor::
get_link () const
{
        return m_link;
}

leviathan::metax::socket_reactor::
socket_reactor(leviathan::metax::link* l)
        : m_link(l)
{
        poco_assert(nullptr != l);
}

leviathan::metax::socket_reactor::
~socket_reactor()
{
        m_link = nullptr;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

