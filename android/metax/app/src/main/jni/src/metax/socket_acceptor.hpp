/**
 * @file src/metax/socket_acceptor.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::socket_acceptor.
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

#ifndef LEVIATHAN_METAX_SOCKET_ACCEPTOR_HPP
#define LEVIATHAN_METAX_SOCKET_ACCEPTOR_HPP


// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/SecureServerSocket.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SocketNotification.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                template <class ServiceHandler>
                struct socket_acceptor;
        }
}


/**
 * @brief This class implements the Acceptor part of the Acceptor-Connector design
 * pattern.
 *
 * The Acceptor-Connector pattern has been described in the book "Pattern
 * Languages of Program Design 3", edited by Robert Martin, Frank Buschmann and
 * Dirk Riehle (Addison Wesley, 1997).
 *
 * The Acceptor-Connector design pattern decouples connection establishment and
 * service initialization in a distributed system from the processing performed
 * once a service is initialized.
 * This decoupling is achieved with three components: Acceptors, Connectors and
 * Service Handlers.
 * The socket_acceptor passively waits for connection requests (usually from a
 * remote Connector) and establishes a connection upon arrival of a connection
 * requests. Also, a Service Handler is initialized to process the data
 * arriving via the connection in an application-specific way.
 *
 * The socket_acceptor sets up a SecureServerSocket and registers itself for a
 * ReadableNotification, denoting an incoming connection request.
 *
 * When the SecureServerSocket becomes readable the socket_acceptor accepts the
 * connection request and creates a ServiceHandler to service the connection.
 *
 * The ServiceHandler class must provide a constructor that takes a
 * SecureStreamSocket and a SocketReactor as arguments, e.g.:
 * MyServiceHandler(const SecureStreamSocket& socket, ServiceReactor& reactor)
 *
 * When the ServiceHandler is done, it must destroy itself.
 *
 * Subclasses can override the createServiceHandler() factory method if special
 * steps are necessary to create a ServiceHandler object.
 */
template <class ServiceHandler>
struct leviathan::metax::socket_acceptor
{
        /// @name Public interface
public:
        /**
         * Registers the socket_acceptor with a SocketReactor.
         * A subclass can override this and, for example, also register an
         * event handler for a timeout event.
         *
         * The overriding method must call the baseclass implementation first.
         */
	virtual void registerAcceptor(Poco::Net::SocketReactor& r);
	
        /**
         * Unregisters the socket_acceptor.
         *
         * A subclass can override this and, for example, also unregister its
         * event handler for a timeout event.
         *
         * The overriding method must call the baseclass implementation first.
         */
	virtual void unregisterAcceptor();
	
	void onAccept(Poco::Net::ReadableNotification* pNotification);

        /// @name Protected functions
protected:
        /**
         * Create and initialize a new ServiceHandler instance.
         *
         * Subclasses can override this method.
         */
	virtual ServiceHandler* createServiceHandler(
                        Poco::Net::SecureStreamSocket& sock);

        /**
         * Returns a pointer to the SocketReactor where this socket_acceptor is
         * registered.
         *
         * The pointer may be null.
         */
        Poco::Net::SocketReactor* reactor();

        /**
         * Returns a reference to the socket_acceptor's socket.
         */
        Poco::Net::Socket& socket();

        /// @name Member data
private:
        Poco::Net::SecureServerSocket   m_socket;
        Poco::Net::SocketReactor* m_pReactor;

        /// @name Special member functions
public:
        /**
         * Creates an socket_acceptor, using the given SecureServerSocket.
         */
	explicit socket_acceptor(Poco::Net::SecureServerSocket& sock);

        /** Creates an socket_acceptor, using the given SecureServerSocket.
         *
         * The socket_acceptor registers itself with the given SocketReactor.
         */
	socket_acceptor(Poco::Net::SecureServerSocket& sock,
                                        Poco::Net::SocketReactor& r);

	/**
         * Destroys the socket_acceptor.
         */
	virtual ~socket_acceptor();

        /// @name Deleted special functions
private:
	socket_acceptor();
	socket_acceptor(const socket_acceptor&);
	socket_acceptor& operator = (const socket_acceptor&);
};

#include "socket_acceptor.txx"

#endif // LEVIATHAN_METAX_SOCKET_ACCEPTOR_HPP

