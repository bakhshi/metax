/**
 * @file src/metax/link_peer.cpp
 *
 * @brief Implementation of the class @ref
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

// Macro Definitions
#if defined(_WIN32)
#define NOMINMAX
#endif

// Headers from this project
#include "link_peer.hpp"
#include "socket_reactor.hpp"
#include "link.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTime.h>
#include <Poco/Net/NetException.h>

// Headers from standard libraries
#include <sstream>
#include <memory>

void leviathan::metax::link_peer::
handle_socket_error()
{
        if (! error_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        disconnect_from_link();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
handle_ping_input()
{
        if (! ping_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        try {
                poco_assert(nullptr != m_socket);
                const link_peer_package& pkg = *ping_rx;
                uint32_t s = 0;
                uint32_t header = s | ((uint32_t)pkg.flags << 24);
                uint32_t n = Poco::ByteOrder::toNetwork(header);
                int c = std::numeric_limits<int>::max();
                c = m_socket->sendBytes((const char*)(&(n)), sizeof(n));
                if (0 > c) {
                        // Todo: handle this case
                }
                ping_rx.consume();
        } catch (...) {
                METAX_ERROR("FAILED to send ack message. - " + m_address);
                disconnect_from_link();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
handle_link_input()
{
        if (! link_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        switch ((*link_rx).cmd) {
                case metax::data: {
                        handle_link_data();
                        break;
                } case metax::shutdown: {
                        disconnect_from_link();
                        break;
                } default: {
                        poco_assert((bool) ! "Command does not handled yet !");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
handle_link_data()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        poco_assert(link_rx.has_data());
        poco_assert(nullptr != m_socket);
        try {
                const link_peer_package& pkg = *link_rx;
                poco_assert(nullptr != m_socket);
                METAX_TRACE("sending by socket to " + m_address);
                send_by_socket(*m_socket, pkg);
                METAX_TRACE("sending by socket completed to " + m_address);
                link_rx.consume();
        } catch (...) {
                METAX_ERROR("FAILED to send Data. Invalid socket - "
                                + m_address);
                disconnect_from_link();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
send_by_socket(Poco::Net::StreamSocket& sock, platform::default_package pkg)
{
        uint32_t header = pkg.size | ((uint32_t)pkg.flags << 24);
        poco_assert(0 != pkg.size);
        uint32_t n = Poco::ByteOrder::toNetwork(header);
        int c = std::numeric_limits<int>::max();
        std::unique_ptr<char[]> buf(
                        new char[sizeof(pkg.size) + pkg.size]);
        memcpy(buf.get(), (const char*)(&(n)), sizeof(pkg.size));
        memcpy(buf.get() + sizeof(pkg.size), pkg.message, pkg.size);
        c = sock.sendBytes(buf.get(), pkg.size + sizeof(pkg.size));
        (void)c;
}

void leviathan::metax::link_peer::
clear_inputs()
{
        // TODO add logic to provide feedback to link that requests are dropped/
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        while(link_rx.has_data()) {
                link_rx.consume();
        }
        while(error_rx.has_data()) {
                error_rx.consume();
        }
        while(ping_rx.has_data()) {
                ping_rx.consume();
        }
        error_tx.deactivate();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
disconnect_from_link()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        clear_inputs();
        (*link_tx).cmd = metax::disconnect;
        (*link_tx).set_payload(std::string("Connection with peer is invalid\0"));
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link_peer::
read_nbytes(Poco::Net::StreamSocket& sock, char* buf, uint32_t size)
{
        poco_assert(nullptr != buf);
        uint32_t offset = 0;
        while (offset != size) {
                int32_t n = (int32_t)sock.receiveBytes(buf + offset,
                                (int)(size - offset));
                if (0 > n) {
                        throw Poco::Net::InvalidSocketException(
                                        "peer socket is closed");
                } else if (0 == n) {
                        throw Poco::Net::NetException("Peer disconnected");
                }
                offset += n;
        }
}

uint32_t leviathan::metax::link_peer::
read_message_size(Poco::Net::StreamSocket& sock)
{
        char buf[4];
        read_nbytes(sock, buf, 4);
        uint32_t * tmp = reinterpret_cast<uint32_t*>(buf);
        uint32_t s = Poco::ByteOrder::fromNetwork(*tmp);
        return s;
}

leviathan::platform::default_package leviathan::metax::link_peer::
read_message(Poco::Net::StreamSocket& sock)
{
        uint32_t header = read_message_size(sock);
        uint32_t message_size = header & 0x00FFFFFF;
        if (1000000000 < message_size) {
                throw Poco::Exception(
                        "Metax shouldn't receive message bigger than 1GB,"
                        "message size is: " +
                        platform::utils::to_string(message_size));
        }
        platform::default_package pkg;
        pkg.flags = (unsigned char)((header & 0xFF000000) >> 24);
        pkg.resize(message_size);
        pkg.message = new char[message_size];
        read_nbytes(sock, pkg.message, message_size);
        return pkg;
}

void leviathan::metax::link_peer::
handle_socket_data()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        platform::default_package pkg = read_message(*m_socket);
        (*link_tx).cmd = metax::data;
        (*link_tx).set_payload(pkg);
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

bool leviathan::metax::link_peer::
configure_socket()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        // Later more deep analysis should be done to define whether the link
        // is duplicate or not, i.e check also SessionID, in order to allow 2
        // different peers beyond the NAT to connect to the same peer.
        poco_assert(nullptr != m_link);
        if (nullptr == m_socket) {
                try {
                        if (m_link->use_ssl) {
                                m_socket = new Poco::Net::SecureStreamSocket(
                                        Poco::Net::SocketAddress(m_address));
                        } else {
                                m_socket = new Poco::Net::StreamSocket(
                                        Poco::Net::SocketAddress(m_address));
                        }
                        m_socket->setSendTimeout(60 * Poco::Timespan::SECONDS);
                } catch (Poco::Exception& e) {
                        METAX_WARNING("Failed to connect " +
                                        m_address + " " + e.displayText());
                        m_link->mark_as_disconnected(this, m_thread);
                        return false;
                } catch (...) {
                        METAX_WARNING("Failed to connect " + m_address);
                        m_link->mark_as_disconnected(this, m_thread);
                        return false;

                }
        }
        m_link->add_peer(this, m_thread);
        send_device_id();
        runnable = new Poco::RunnableAdapter<link_peer>(*this,
                                        &link_peer::handle_socket_input);
        m_read_thread.start(*runnable);
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
        // start negotiation
        return true;
}

void leviathan::metax::link_peer::
shutdown_link_peer()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        try {
                m_socket->shutdown();
        } catch (...) {
                METAX_DEBUG("Socket not connected");
        }
        m_socket->close();
        m_read_thread.join();
        poco_assert(nullptr != m_link);
        finish();
        if (! m_link->isCancelled()) {
                m_link->remove_peer(this);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
runTask()
try {
        if (! configure_socket()) {
                return;
        }
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        //Start negotiation
        while (true) {
                if (! wait()) {
                        break;
                }
                handle_ping_input();
                handle_link_input();
                handle_socket_error();
        }
        shutdown_link_peer();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch(const Poco::Exception& e) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL(e.displayText());
        std::terminate();
} catch(...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

void leviathan::metax::link_peer::
handle_socket_input()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        poco_assert(nullptr != m_socket);
        try {
                while (true) {
                        handle_socket_data();
                }
        } catch (const Poco::Exception& e) {
                METAX_WARNING(e.displayText() + " - " + m_address);
        } catch (...) {
        }
        handle_socket_disconnection();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

void leviathan::metax::link_peer::
handle_socket_disconnection()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        poco_assert(nullptr != m_socket);
        *error_tx = true;
        error_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

std::string
leviathan::metax::link_peer::
address() const
{
        return m_address;
}

bool leviathan::metax::link_peer::
is_my_connection() const
{
        return m_my_connection;
}

void leviathan::metax::link_peer::
send_device_id()
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        poco_assert(nullptr != m_link);
        link_data ld;
        ld.cmd = metax::device_id;
        ld.resize(m_link->device_id.size());
        char* tmp = new char[m_link->device_id.size()];
        std::copy(m_link->device_id.begin(), m_link->device_id.end(), tmp);
        ld.message = tmp;
        ld.public_key = m_link->public_key;
        ld.load_info = m_link->peers.size();
        platform::default_package pkg;
        pkg.flags = platform::default_package::LINK_MSG;
        ld.serialize(pkg);
        try {
                poco_assert(nullptr != m_socket);
                Poco::Timespan st = m_socket->getSendTimeout();
                Poco::Timespan rt = m_socket->getReceiveTimeout();
                m_socket->setSendTimeout(Poco::Timespan(5, 0));
                m_socket->setReceiveTimeout(Poco::Timespan(5, 0));
                METAX_TRACE("sending by socket to " + m_address);
                send_by_socket(*m_socket, pkg);
                METAX_TRACE("sending by socket completed to " + m_address);
                m_socket->setSendTimeout(st);
                m_socket->setReceiveTimeout(rt);
        } catch (...) {
                METAX_ERROR("FAILED to send Data. Invalid socket - "
                                + m_address);
                disconnect_from_link();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
}

leviathan::metax::link_peer::
link_peer(Poco::Net::StreamSocket& socket,
                Poco::Net::SocketReactor& reactor)
        : task(socket.peerAddress().toString() + " handler",
                        Poco::Logger::get("metax.link_peer"), false)
        , link_rx(this)
        , link_tx(this)
        , error_rx(this)
        , error_tx(this)
        , ping_rx(this)
        , is_sync_finished(true)
        , m_id(LINK_REQUEST_UPPER_BOUND)
        , m_gid(-1)
        , m_reactor(dynamic_cast<socket_reactor&>(reactor))
        , m_my_connection(false)
        , m_link(nullptr)
        , m_thread(new Poco::Thread)
        , m_read_thread()
        , runnable(nullptr)
{
        METAX_INFO(m_address + " connection from");
        platform::csp::connect(error_rx, error_tx);
        m_link = m_reactor.get_link();
        if (m_link->use_ssl) {
                m_socket = new Poco::Net::SecureStreamSocket(socket);
        } else {
                m_socket = new Poco::Net::StreamSocket(socket);
        }
        m_address = m_socket->peerAddress().toString();
        m_thread->start(*this);
}

leviathan::metax::link_peer::
link_peer(const std::string& ipport, int gid,
                Poco::Net::SocketReactor& reactor)
        : task(ipport + " handler", Poco::Logger::get("metax.link_peer"), false)
        , link_rx(this)
        , link_tx(this)
        , error_rx(this)
        , error_tx(this)
        , ping_rx(this)
        , is_sync_finished(true)
        , m_id(LINK_REQUEST_UPPER_BOUND)
        , m_gid(gid)
        , m_reactor(dynamic_cast<socket_reactor&>(reactor))
        , m_socket(nullptr)
        , m_address(ipport)
        , m_my_connection(true)
        , m_link(nullptr)
        , m_thread(new Poco::Thread)
        , m_read_thread()
        , runnable(nullptr)
{
        METAX_INFO("Connect to " + ipport);
        platform::csp::connect(error_rx, error_tx);
        m_link = m_reactor.get_link();
        m_thread->start(*this);
}

leviathan::metax::link_peer::
~link_peer()
try
{
        METAX_TRACE(__FUNCTION__ + (" - " + m_address));
        if (nullptr != m_socket) {
                delete m_socket;
        }
        m_socket = nullptr;
        delete runnable;
        METAX_TRACE(std::string("END ") + __FUNCTION__ + (" - " + m_address));
} catch (Poco::Exception& e) {
        std::cerr << e.displayText() << std::endl;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

