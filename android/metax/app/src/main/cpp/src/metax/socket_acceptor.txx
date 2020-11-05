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
#include "socket_acceptor.hpp"

// Headers from other projects

// Headers from third party libraries

// Headers from standard libraries

template <typename ServiceHandler>
void leviathan::metax::socket_acceptor<ServiceHandler>::
registerAcceptor(Poco::Net::SocketReactor& r)
{
        m_pReactor = &r;
        m_pReactor->addEventHandler(m_socket,
                        Poco::Observer<socket_acceptor,
                        Poco::Net::ReadableNotification>(*this,
                                        &socket_acceptor::onAccept));
}

template <typename ServiceHandler>
void leviathan::metax::socket_acceptor<ServiceHandler>::
unregisterAcceptor()
{
        if (m_pReactor) {
                m_pReactor->removeEventHandler(m_socket,
                        Poco::Observer<socket_acceptor,
                        Poco::Net::ReadableNotification>(*this,
                                &socket_acceptor::onAccept));
        }
}
	
template <typename ServiceHandler>
void leviathan::metax::socket_acceptor<ServiceHandler>::
onAccept(Poco::Net::ReadableNotification* pNotification)
{
        pNotification->release();
        Poco::Net::SecureStreamSocket sock = m_socket.acceptConnection();
        createServiceHandler(sock);
}
	
template <typename ServiceHandler>
ServiceHandler* leviathan::metax::socket_acceptor<ServiceHandler>::
createServiceHandler(Poco::Net::SecureStreamSocket& sock)
{
        return new ServiceHandler(sock, *m_pReactor);
}

template <typename ServiceHandler>
Poco::Net::SocketReactor* leviathan::metax::socket_acceptor<ServiceHandler>::
reactor()
{
        return m_pReactor;
}

template <typename ServiceHandler>
Poco::Net::Socket& leviathan::metax::socket_acceptor<ServiceHandler>::
socket()
{
        return m_socket;
}

template <typename ServiceHandler>
leviathan::metax::socket_acceptor<ServiceHandler>::
socket_acceptor(Poco::Net::SecureServerSocket& sock):
        m_socket(sock),
        m_pReactor(0)
{}

template <typename ServiceHandler>
leviathan::metax::socket_acceptor<ServiceHandler>::
socket_acceptor(Poco::Net::SecureServerSocket& sock,
                        Poco::Net::SocketReactor& r):
        m_socket(sock),
        m_pReactor(0)
{
        registerAcceptor(r);
}

template <typename ServiceHandler>
leviathan::metax::socket_acceptor<ServiceHandler>::
~socket_acceptor()
{
        unregisterAcceptor();
}
	
// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

