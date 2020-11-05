/**
 * @file src/metax/link_peer.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::link_peer.
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

#ifndef LEVIATHAN_METAX_LINK_PEER_HPP
#define LEVIATHAN_METAX_LINK_PEER_HPP


// Headers from this project
#include "socket_reactor.hpp"
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>

// Headers from third party libraries
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/NotificationQueue.h>
#include <Poco/RunnableAdapter.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct link_peer;
                struct link_notification;
        }
}

/**
 * @brief Manages connection with one peer.
 * 
 */
struct leviathan::metax::link_peer
        : public leviathan::platform::csp::task
{
        /// @name Public member variables for I/O
public:
        platform::csp::input<link_peer_package> link_rx;
        platform::csp::output<link_peer_package> link_tx;
        platform::csp::input<bool> error_rx;
        platform::csp::output<bool> error_tx;
        platform::csp::input<link_peer_package> ping_rx;

        bool is_sync_finished;
        ID32 m_id;
        int m_gid;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

        /**
         * @brief Returns the address of the peer in "ip:port" format string
         *
         * @return - ip address and port of the peer socket in string
         */
        std::string address() const;

        /**
         * @brief Checks whether this Metax is the initiator of the connection
         *
         * @return - true if this Metax initiated the socket connection, false
         * otherwise
         */
        bool is_my_connection() const;

        /// @name Private data
private:
        socket_reactor& m_reactor;
        Poco::Net::StreamSocket* m_socket;
        std::string m_address;
        bool m_my_connection;
        link* m_link;
        Poco::Thread* m_thread;
        Poco::Thread m_read_thread;
        Poco::RunnableAdapter<link_peer>* runnable;

        /// @name Private helper functions
private:
        void handle_link_input();
        void handle_link_data();
        void handle_socket_input();
        bool configure_socket();
        void handle_disconnect_event();
        void disconnect_from_link();
        void handle_socket_data();
        void clear_inputs();
        void shutdown_link_peer();
        void handle_socket_error();
        void handle_ping_input();
        void handle_socket_disconnection();
        void send_device_id();

        /// @name Public static functions
public:
        static void send_by_socket(Poco::Net::StreamSocket& sock,
                                        platform::default_package pkg);
        static void read_nbytes(Poco::Net::StreamSocket& sock,
                                        char* buf, uint32_t size);
        static uint32_t read_message_size(Poco::Net::StreamSocket& sock);
        static platform::default_package
                        read_message(Poco::Net::StreamSocket& sock);

        /// @name Special member functions
public:
        /**
         * Constructor
         *
         * @param socket - already connected socket a the peer
         * @param reactor - reactor where socket should be registered to handle
         * events
         */
        link_peer(Poco::Net::StreamSocket& socket,
                        Poco::Net::SocketReactor& reactor);

        /**
         * Constructor
         *
         * @param ip - IP address to which connection should be established
         * @param gid - group id of the peer, used by balancer, to leave only
         * one connection with the same group id
         * @param reactor - reactor where socket should be registered to handle
         * events
         *
         */
        link_peer(const std::string& ip, int gid,
                        Poco::Net::SocketReactor& reactor);

        /// Destructor
        ~link_peer();
        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        link_peer(const link_peer&) = delete;

        /// This class is not assignable
        link_peer& operator=(const link_peer&) = delete;

}; // class leviathan::metax::link_peer

#endif // LEVIATHAN_METAX_LINK_PEER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

