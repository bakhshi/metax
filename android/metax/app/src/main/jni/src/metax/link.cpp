/**
 * @file
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::link.
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
#include "link.hpp"
#include "ssl_password_handler.hpp"
#include "configuration_manager.hpp"

// Headers from other projects
#include <platform/fifo.hpp>
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/MD5Engine.h>
#include <Poco/DigestStream.h>
#include <Poco/FileStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Net/SecureServerSocket.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/ConsoleCertificateHandler.h>
#ifndef __SH4__
#include <Poco/Net/NetworkInterface.h>
#endif

#if defined(__APPLE__)
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

#if defined(__SH4__) || defined(ANDROID)
#include <net/if.h>
#endif

// Headers from standard libraries
#include <sstream>
#include <fstream>
#include <thread>

leviathan::metax::ID32
leviathan::metax::link::
get_peer_id()
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = m_request_counter++;
        if (LINK_REQUEST_UPPER_BOUND <= m_request_counter) {
                m_request_counter = LINK_REQUEST_LOWER_BOUND;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return id;
}

void leviathan::metax::link::
configure()
{
        METAX_TRACE(__FUNCTION__);
        m_reactor_thread.start(m_reactor);
        m_connection_guard.start(m_on_timer_callback);
        m_keep_alive.start(m_on_keep_alive_call);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_bind_request()
{
        METAX_TRACE(__FUNCTION__);
        std::stringstream ss;
        int port;
        ss << (*config_rx).message;
        ss >> port;
        if (0 > port) {
                METAX_WARNING("Peer server socket is disabled." );
                m_reactor.stop();
                return;
        }
        try {
                if (use_ssl) {
                        Poco::Net::SecureServerSocket* ssock =
                                new Poco::Net::SecureServerSocket(port);
                        m_server_socket = ssock;
                        m_secure_acceptor = new socket_acceptor<link_peer>(
                                                *ssock, m_reactor);
                } else {
                        m_server_socket = new Poco::Net::ServerSocket(port);
                        m_acceptor = new Poco::Net::SocketAcceptor<link_peer>(
                                                *m_server_socket, m_reactor);
                }
        } catch (Poco::Exception& e) {
                METAX_ERROR("Failed to init server socket: " +
                                e.displayText());
        } catch (...) {
                METAX_ERROR("Failed to init server socket");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_user_public_key(const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        std::stringstream oss;
        Poco::FileInputStream istr(path);
        Poco::StreamCopier::copyStream(istr, oss);
        public_key = oss.str();
        platform::utils::trim(public_key);
        METAX_DEBUG("Received user public key : " + public_key);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_device_id(const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        Poco::MD5Engine md5;
        md5.update(platform::utils::read_file_content(path, true));
        device_id = std::move(md5.digest());
        METAX_DEBUG("Received device id from configuration manager: "
                        + Poco::DigestEngine::digestToHex(device_id));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
validate_peers_rating(const Poco::JSON::Object::Ptr& obj)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::ConstIterator b = obj->begin();
        for (; b != obj->end(); ++b) {
                if (b->second.isInteger() && "protocol_version" == b->first) {
                        continue;
                }
                if (b->second.isEmpty()) {
                        throw Poco::Exception(
                                   "Invalid peers rating configuration");
                }
                Poco::JSON::Object::Ptr o =
                        b->second.extract<Poco::JSON::Object::Ptr>();
                poco_assert(nullptr != o);
                if (o->has("rating") && ! o->get("rating").isInteger()) {
                        throw Poco::Exception("Invalid rating value");
                }
                if (o->has("last_seen") && ! o->get("last_seen").isInteger()) {
                        throw Poco::Exception("Invalid last_seen value");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_peers_ratings(const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        m_peers_rating_path = path;
        std::ifstream ifs(m_peers_rating_path);
        if (! ifs.is_open()) {
                METAX_WARNING("Invalid peers rating config file.");
                return;
        }
        try {
                using P = Poco::JSON::Object::Ptr;
                P obj = platform::utils::parse_json<P>(ifs);
                validate_peers_rating(obj);
                m_peers_rating = obj;
                if (! m_peers_rating->has("protocol_version")) {
                        m_peers_rating->set("protocol_version", 1);
                        save_peers_rating_in_file();
                }
        } catch (const Poco::Exception& e) {
                METAX_WARNING("Could not parse peers rating configuration: "
                                + e.displayText());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_user_keys(const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        m_user_key_path = path;
        if (! Poco::File(m_user_key_path + "/" + m_pbkey_filename).exists()) {
                METAX_WARNING("Invalid user key path.");
                return;
        }
        if (! Poco::File(m_user_key_path + "/" + m_prkey_filename).exists()) {
                METAX_WARNING("Invalid user key path.");
                return;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_user_json_info(const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        m_user_json_info_path = path;
        if (! Poco::File(m_user_json_info_path).exists()) {
                METAX_WARNING("Invalid user json info file.");
                return;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_peer_verification_mode(const std::string& mode)
{
        METAX_TRACE(__FUNCTION__);
        if ("strict" == mode) {
                m_peer_verification_mode = verification_mode::strict;
        } else if ("relaxed" == mode) {
                m_peer_verification_mode = verification_mode::relaxed;
        } else {
                poco_assert("none" == mode);
                m_peer_verification_mode = verification_mode::none;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_device_id_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::device_id == in.cmd);
        poco_assert(nullptr != in.message.get());
        Poco::JSON::Array::Ptr arr = platform::utils::
                parse_json<Poco::JSON::Array::Ptr>(in.message.get());
        poco_assert(9 == arr->size());
        m_device_key_path = arr->getElement<std::string>(0);
        handle_device_id(m_device_key_path + "/" + m_pbkey_filename);
        handle_user_public_key(arr->getElement<std::string>(1));
        handle_peers_ratings(arr->getElement<std::string>(2));
        std::istringstream iss(arr->getElement<std::string>(3));
        std::istringstream iss1(arr->getElement<std::string>(4));
        iss >> m_peers_default_rating;
        iss1 >> m_sync_class;
        METAX_INFO("Received peers default rating from config: " +
                        arr->getElement<std::string>(3));
        handle_user_keys(arr->getElement<std::string>(5));
        handle_user_json_info(arr->getElement<std::string>(6));
        use_ssl = arr->getElement<bool>(7);
        handle_peer_verification_mode(arr->getElement<std::string>(8));
        initialize_server_ssl_manager();
        initialize_client_ssl_manager();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::JSON::Array::Ptr leviathan::metax::link::
get_online_peers_list()
{
        Poco::JSON::Array::Ptr obj = new Poco::JSON::Array();
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto i : peers) {
                peer_io* io = i.second;
                poco_assert(nullptr != io);
                if (! io->device_id.empty()) {
                        Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
                        p->set("user_public_key", io->public_key);
                        p->set("device_id", Poco::DigestEngine::
                                        digestToHex(io->device_id));
                        p->set("address", io->lp->address());
                        obj->add(p);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return obj;
}

void leviathan::metax::link::
handle_config_get_online_peers_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::get_online_peers == in.cmd);
        Poco::JSON::Array::Ptr obj = get_online_peers_list();
        poco_assert(nullptr != obj);
        std::stringstream ss;
        obj->stringify(ss);
        link_cfg_package& out = *config_tx;
        out.cmd = metax::get_online_peers_response;
        out.request_id = in.request_id;
        out.set_payload(ss.str());
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_reconnect_to_peers()
{
        METAX_TRACE(__FUNCTION__);
        m_connection_guard.restart(0);
        handle_dropped_connections();
        m_connection_guard.restart(CONNECTION_GUARD_TIMEOUT);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
save_start_pairing_request_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::start_pairing == in.cmd);
        poco_assert(nullptr != in.message.get());
        std::string payload = in.message.get();
        m_pairing_timout = platform::utils::from_string<uint16_t>(payload);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
initialize_server_ssl_manager()
{
        METAX_TRACE(__FUNCTION__);
        namespace N = Poco::Net;
        typedef N::PrivateKeyPassphraseHandler P;
        P* p = new ssl_password_handler(true);
        typedef N::InvalidCertificateHandler I;
        I* i = new N::ConsoleCertificateHandler(true);
        std::string pk = m_device_key_path + "/" + m_prkey_filename;
        std::string pc = m_device_key_path + "/" + m_cert_filename;
        N::Context::Ptr c = new N::Context(
                N::Context::SERVER_USE, pk, pc, "",
                N::Context::VERIFY_NONE, 9, false,
                "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
        N::SSLManager::instance().initializeServer(p, i, c);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
create_pairing_tcp_server_socket()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr == m_pairing_socket);
        namespace N = Poco::Net;
        N::SecureServerSocket s(m_pairing_port);
        N::TCPServerParams* p = new N::TCPServerParams();
        p->setThreadIdleTime(1);
        m_pairing_socket = new N::TCPServer(
                        new newConnectionFactory(this), s, p);
        m_pairing_socket->start();
        METAX_INFO(std::string("Transfer keys: Start server."));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax::link::
generate_code(uint8_t l)
{
        METAX_TRACE(__FUNCTION__);
        srand(time(NULL));
        static const char alphanum[] =
               "0123456789"
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "abcdefghijklmnopqrstuvwxyz";
        std::string s = "";
        for (uint8_t i = 0; i < l; ++i) {
                char a = alphanum[rand() % (sizeof(alphanum) - 1)];
                s+= a;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return s;
}

void leviathan::metax::link::
send_fail_message(const std::string& m, metax::command cmd)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::start_pairing == in.cmd);
        Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
        std::string fail =
                metax::start_pairing_fail == cmd ? "error" : "warning";
        p->set(fail, m);
        std::stringstream ss;
        p->stringify(ss);
        link_cfg_package& out = *config_tx;
        out.cmd = cmd;
        out.request_id = in.request_id;
        out.set_payload(ss.str());
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_generated_code()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
        poco_assert(nullptr != m_pairing_socket);
        m_generated_code = generate_code(7);
        poco_assert(! m_generated_code.empty());
        p->set("generated_code", m_generated_code);
        std::stringstream ss;
        p->stringify(ss);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::start_pairing == in.cmd);
        link_cfg_package& out = *config_tx;
        out.cmd = metax::get_generated_code;
        out.request_id = in.request_id;
        out.set_payload(ss.str());
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
on_expired_start_pairing(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr != m_pairing_socket) {
                m_pairing_socket->stop();
                delete m_pairing_socket;
                m_pairing_socket = nullptr;
                m_generated_code = "";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_start_pairing()
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr != m_pairing_socket) {
                send_fail_message("Server already started",
                                        metax::get_generated_code);
                return;
        }
        try {
                save_start_pairing_request_info();
                create_pairing_tcp_server_socket();
                send_generated_code();
                poco_assert(nullptr != m_pairing_socket);
                m_start_pairing_timer.stop();
                Poco::TimerCallback<link> start_pairing_callback(*this,
                                &link::on_expired_start_pairing);
                m_start_pairing_timer.setStartInterval(1000 * m_pairing_timout);
                m_start_pairing_timer.start(start_pairing_callback,
                                Poco::ThreadPool::defaultPool());
        } catch (const Poco::Exception& e) {
                METAX_INFO(std::string("Transfer keys: Fail to start server ")
                                                + std::string(e.displayText()));
                send_fail_message("Transfer keys: Fail to start server "
                                + e.displayText(), metax::start_pairing_fail);
        } catch (...) {
                send_fail_message("Transfer keys: Fail to start server ",
                                                metax::start_pairing_fail);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
cancel_pairing()
{
        METAX_TRACE(__FUNCTION__);
        m_start_pairing_timer.stop();
        if (nullptr != m_pairing_socket) {
                m_pairing_socket->stop();
                delete m_pairing_socket;
                m_pairing_socket = nullptr;
                m_generated_code = "";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
initialize_client_ssl_manager()
{
        METAX_TRACE(__FUNCTION__);
        namespace N = Poco::Net;
        typedef N::PrivateKeyPassphraseHandler P;
        P* p = new ssl_password_handler(false);
        typedef N::InvalidCertificateHandler I;
        I* i = new N::ConsoleCertificateHandler(true);
        N::Context::Ptr c = new N::Context(
                N::Context::CLIENT_USE, "", "", "",
                N::Context::VERIFY_NONE, 9, false,
                "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
        N::SSLManager::instance().initializeClient(p, i, c);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

uint32_t leviathan::metax::link::
address_to_int(const std::string& s)
{
        METAX_TRACE(__FUNCTION__);
        size_t index_begin = 0;
        size_t index_end = s.find_first_of('.', index_begin);
        uint32_t n = 0;
        while (std::string::npos != index_end) {
                std::string p = s.substr(index_begin, index_end);
                n |= platform::utils::from_string<uint32_t>(p);
                n <<= 8;
                index_begin = index_end + 1;
                index_end = s.find_first_of('.', index_begin);
        }
        n |= platform::utils::from_string<uint32_t>(s.substr(index_begin));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return n;
}

std::string leviathan::metax::link::
int_to_address(uint32_t n)
{
        METAX_TRACE(__FUNCTION__);
        std::string s = ""; 
        for (int i = 0; i < 3; ++i) {
                uint32_t x = n & 0xff;
                s = '.' + platform::utils::to_string(x) + s;
                n >>= 8;
        }
        s = platform::utils::to_string(n) + s;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return s;
}


#if defined(__SH4__) || defined(ANDROID)
void leviathan::metax::link::
get_info_from_system(int fd, struct ifreq* ifr,
                std::vector<std::pair<std::string, std::string>>& v)
{
        METAX_TRACE(__FUNCTION__);
        typedef struct sockaddr_in S;
        struct ifreq netmask = {};
        strcpy(netmask.ifr_name, ifr->ifr_name);
        if (ioctl(fd, SIOCGIFNETMASK, &netmask) != 0) {
                return;
        }
        S* s = (S *)&ifr->ifr_addr;
        std::string ip = inet_ntoa(s->sin_addr);
        s = (S *)&netmask.ifr_addr;
        std::string mask = inet_ntoa(s->sin_addr);
        v.push_back(std::pair<std::string, std::string>(ip, mask));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
get_local_ip_addresses_and_subnets(
                std::vector<std::pair<std::string, std::string>>& v)
{
        METAX_TRACE(__FUNCTION__);
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
                return;
        }
        char buffer[2048] = {};
        struct ifconf {
                size_t ifc_len;
                void* ifc_buffer;
        };
        ifconf ifc = { sizeof(buffer), buffer };
        if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
                return;
        }
        struct ifreq* ifr;
        for (size_t i = 0; i < ifc.ifc_len; i += sizeof(*ifr)) {
                ifr = (struct ifreq *)(buffer + i);
                if (ifr->ifr_addr.sa_family == AF_INET) {
                        std::string n = ifr->ifr_name;
                        if (n.find("wlan") != std::string::npos) {
                                get_info_from_system(fd, ifr, v);
                        }
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

#elif defined(__APPLE__)
void leviathan::metax::link::
get_local_ip_addresses_and_subnets(
                std::vector<std::pair<std::string, std::string>>& v)
{
        METAX_TRACE(__FUNCTION__);
        struct ifaddrs *interfaces = NULL;
        int success = getifaddrs(&interfaces);
        if (success != 0) {
                return;
        }
        struct ifaddrs *temp_addr = interfaces;
        typedef struct sockaddr_in S;
        typedef std::pair<std::string, std::string> P;
        while (temp_addr != NULL) {
                if (temp_addr->ifa_addr->sa_family == AF_INET) {
                        std::string n = temp_addr->ifa_name;
                        if (n.find("en") != std::string::npos) {
                                S* s = (S *)temp_addr->ifa_addr;
                                std::string ip = inet_ntoa(s->sin_addr);
                                s = (S *)temp_addr->ifa_netmask;
                                std::string mask = inet_ntoa(s->sin_addr);
                                v.push_back(P(ip, mask));
                        }
                }
                temp_addr = temp_addr->ifa_next;
        }
        freeifaddrs(interfaces);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}
#else
void leviathan::metax::link::
get_local_ip_addresses_and_subnets(
                std::vector<std::pair<std::string, std::string>>& v)
{
        METAX_TRACE(__FUNCTION__);
        std::string ip = "";
        std::string mask = "";
        using NI = Poco::Net::NetworkInterface;
        NI::Map m = NI::map(false, false);
        typedef std::pair<std::string, std::string> P;
        for (auto i = m.begin(); i != m.end(); ++i) {
                NI& ni = i->second;
                std::string n = ni.displayName();
                if (n.find("en") != std::string::npos ||
                            n.find("wl") != std::string::npos ) {
                        ip = ni.address().toString();
                        mask = ni.subnetMask().toString();
                        v.push_back(P(ip, mask));
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}
#endif

bool leviathan::metax::link::
create_connection(const std::string& current_ip)
{
        METAX_TRACE(__FUNCTION__);
        std::string current_address = current_ip + ":" +
                                platform::utils::to_string(m_pairing_port);
        namespace N = Poco::Net;
        try {
                const N::SocketAddress sa(current_address);
                N::SecureStreamSocket ss;
                Poco::Timespan t(0, 70000);
                ss.connect(sa, t);
                METAX_INFO(std::string("Transfer keys: Connected to ") +
                        current_address);
                ss.close();
                return true;
        } catch (const N::NetException& e) {
                METAX_INFO(std::string("Transfer keys: Fail to connect ") +
                        current_address + std::string(e.displayText()));
                return false;
        } catch (const Poco::TimeoutException& e) {
                METAX_INFO(std::string("Transfer keys: Timeout to connect ") +
                        current_address + std::string(e.displayText()));
                return false;
        }
}

void leviathan::metax::link::
collect_available_ips_in_specified_network(const std::string& ip,
                        const std::string& mask, std::set<std::string>& a)
{
        METAX_TRACE(__FUNCTION__);
        if (ip.empty() || mask.empty() || (mask.substr(0, 7) != "255.255")) {
                return;
        }
        uint32_t int_ip = address_to_int(ip);
        uint32_t int_subnet = address_to_int(mask);
        uint32_t start_ip = (int_ip & int_subnet) + 1;
        uint32_t end_ip = (int_ip | (~ int_subnet)) - 1;
        for (uint32_t i = start_ip; i <= end_ip; ++i) {
                std::string current_ip = int_to_address(i);
                bool b = create_connection(current_ip);
                if (b) {
                        a.insert(current_ip);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::JSON::Array::Ptr leviathan::metax::link::
get_pairing_peers_list()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Array::Ptr obj = new Poco::JSON::Array();
        std::vector<std::pair<std::string, std::string>> v;
        get_local_ip_addresses_and_subnets(v);
        std::set<std::string> a;
        for (uint8_t j = 0; j < v.size(); ++j) {
                collect_available_ips_in_specified_network(
                        v[j].first, v[j].second, a);
        }
        int  peers_count = 0;
        for (const auto& ip : a) {
                Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
                p->set(platform::utils::to_string(
                                        ++peers_count), ip);
                obj->add(p);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return obj;
}

void leviathan::metax::link::
handle_config_get_pairing_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::get_pairing_peers == in.cmd);
        Poco::JSON::Array::Ptr obj = get_pairing_peers_list();
        poco_assert(nullptr != obj);
        std::stringstream ss;
        obj->stringify(ss);
        link_cfg_package& out = *config_tx;
        out.cmd = metax::get_pairing_peers_response;
        out.request_id = in.request_id;
        out.set_payload(ss.str());
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax::link::
get_ip_from_request_keys_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(nullptr != in.message.get());
        std::string payload = in.message.get();
        size_t index = payload.find_first_of(',');
        poco_assert(std::string::npos != index);
        std::string ip = payload.substr(0, index);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return ip;
}

std::string leviathan::metax::link::
get_code_from_request_keys_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(nullptr != in.message.get());
        std::string payload = in.message.get();
        size_t index = payload.find_first_of(',');
        poco_assert(std::string::npos != index);
        poco_assert(std::string::npos != index + 1);
        std::string code = payload.substr(index + 1);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return code;
}

void leviathan::metax::link::
save_user_keys(Poco::Net::SecureStreamSocket& client, ID32 req_id)
{
        platform::default_package pkg = link_peer::read_message(client);
        key_transfer_package ktp;
        ktp.extract_header(pkg);
        link_key_manager_package& out = (*key_manager_tx);
        out.extract_header(pkg);
        out.request_id = req_id;
        out.cmd = metax::request_keys_response;
        key_manager_tx.commit();
}

void leviathan::metax::link::
connect_server_and_save_keys(const std::string& current_address,
                const std::string& code, ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        const Poco::Net::SocketAddress sa(current_address);
        Poco::Net::SecureStreamSocket client;
        Poco::Timespan t(1, 0);
        client.connect(sa, t);
        platform::default_package hp = link_peer::read_message(client);
        key_transfer_package hm;
        hm.extract_payload(hp, hm.header_size());
        if (hm.size == 0) {
                throw Poco::Exception("Received empty message");
        }
        if ("Connected" == std::string(hm.message.get(), hm.size)) {
                key_transfer_package ktp;
                ktp.code = code;
                platform::default_package cm;
                ktp.serialize(cm);
                link_peer::send_by_socket(client, cm);
                save_user_keys(client, req_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_request_keys_fail_to_config(const std::string err, ID32 req_id)
{
        link_cfg_package& out = *config_tx;
        out.request_id = req_id;
        out.cmd = metax::request_keys_fail;
        out.set_payload(err);
        config_tx.commit();
}

void leviathan::metax::link::
handle_config_request_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::request_keys == in.cmd);
        const std::string& ip = get_ip_from_request_keys_request();
        const std::string& code = get_code_from_request_keys_request();
        std::string c = ip + ":" + platform::utils::to_string(m_pairing_port);
        try {
                connect_server_and_save_keys(c, code, in.request_id);
        } catch (const Poco::Net::NetException& e) {
                std::string m = "Transfer keys: Fail to connect " +
                        c + std::string(e.displayText());
                send_request_keys_fail_to_config(m, in.request_id);
        } catch (const Poco::TimeoutException& e) {
                std::string m = "Transfer keys: Timeout to connect " +
                        c + std::string(e.displayText());
                send_request_keys_fail_to_config(m, in.request_id);
        } catch (...) {
                send_request_keys_fail_to_config("Failed to transfer keys",
                                in.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_peer_verified()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::peer_verified == in.cmd);
        auto it = peers.find(in.peer_id);
        if (peers.end() == it) {
                return;
        }
        peer_io* p = it->second;
        accept_connection_with_balancing(p, in.device_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_peer_not_verified()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        const link_cfg_package& in = *config_rx;
        poco_assert(metax::peer_not_verified == in.cmd);
        auto it = peers.find(in.peer_id);
        if (peers.end() != it) {
                send_shutdown_message_to_peer(it->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_connect_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        poco_assert(nullptr != (*config_rx).message.get());
        new link_peer((*config_rx).message.get(),
                                (*config_rx).peer_id, m_reactor);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_config_input()
{
        if (! config_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_config_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        config_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
process_config_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        switch ((*config_rx).cmd) {
                case metax::device_id: {
                        handle_config_device_id_request();
                        break;
                } case metax::bind: {
                        handle_bind_request();
                        break;
                } case metax::connect: {
                        handle_connect_request();
                        break; 
                } case metax::get_online_peers: {
                        handle_config_get_online_peers_request();
                        break;
                } case metax::reconnect_to_peers: {
                        std::thread t(&link::handle_config_reconnect_to_peers, this);
                        t.detach();
                        break;
                } case metax::start_pairing: {
                        handle_config_start_pairing();
                        break;
                } case metax::cancel_pairing: {
                        cancel_pairing();
                        break;
                } case metax::get_pairing_peers: {
                        handle_config_get_pairing_peers();
                        break;
                } case metax::request_keys: {
                        handle_config_request_keys();
                        break;
                } case metax::peer_verified: {
                        handle_config_peer_verified();
                        break;
                } case metax::peer_not_verified: {
                        handle_config_peer_not_verified();
                        break;
                } default : {
                        command c = (*config_rx).cmd;
                        METAX_WARNING(
                                "Message from config is not handled yet:  : "
                                + platform::utils::to_string(c));
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_empty_data(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        const link_peer_package& in = *(p->in);
        if (in.flags == platform::default_package::PING) {
                METAX_INFO("received ping from peer, responding with pong");
                platform::default_package pkg;
                pkg.flags = platform::default_package::PONG;
                *(p->ping_tx) = pkg;
                (p->ping_tx).commit();
        } else if (in.flags != platform::default_package::PONG) {
                send_shutdown_message_to_peer(p);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_link_peer_data(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        const link_peer_package& in = *(p->in);
        p->last_received_time.update();
        p->ping_sent = false;
        if (0 == in.size) {
                handle_empty_data(p);
        } else {
                if (in.flags == platform::default_package::LINK_MSG) {
                        handle_link_message(p);
                } else {
                        handle_router_message_of_link_peer(p);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
configure_peer_by_device_id(leviathan::metax::link::peer_io* p,
                const std::string& s)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p); poco_assert(nullptr != m_peers_rating);
        p->rating = m_peers_default_rating;
        Poco::JSON::Object::Ptr pi;
        if (m_peers_rating->has(s)) {
                pi = m_peers_rating->getObject(s);
                if (pi->has("rating")) {
                        p->rating = pi->getValue<int>("rating");
                }
        } else {
                pi = new Poco::JSON::Object();
        }
        if (1 == m_sync_class) {
                post_send_journal_info_from_last_seen(p->out, pi);
        } else {
                poco_assert(0 == m_sync_class);
                post_kernel_start_storage_sync(p, pi);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
post_kernel_start_storage_sync(leviathan::metax::link::peer_io* p,
                Poco::JSON::Object::Ptr pi)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        poco_assert(nullptr != pi);
        router_link_package& out = (*router_tx);
        out.cmd = metax::received_data;
        poco_assert(nullptr != p->lp);
        out.peers.insert(p->lp->m_id);
        router_data d;
        d.cmd = metax::start_storage_sync;
        d.last_updated = get_peer_last_seen(pi);
        d.serialize(out);
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::UInt64 leviathan::metax::link::
get_peer_last_seen(Poco::JSON::Object::Ptr pi) const
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pi);
        Poco::Timestamp t;
        if (pi->has("last_seen")) {
                t = pi->getValue<Poco::UInt64>("last_seen");
        } else {
                Poco::Timespan ts(3, 0, 0, 0, 0); // three days
                t -= ts.totalMicroseconds();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return t.epochMicroseconds();
}

void leviathan::metax::link::
post_send_journal_info_from_last_seen(
                platform::csp::output<link_peer_package>& out,
                Poco::JSON::Object::Ptr pi)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pi);
        router_data d;
        d.cmd = metax::send_journal_info;
        d.last_updated = get_peer_last_seen(pi);
        d.serialize(*out);
        (*out).cmd = metax::data;
        out.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_notification_to_router(peer_io* p, command cmd)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        poco_assert(metax::connected == cmd || metax::disconnected == cmd);
        if (p->device_id.empty()) {
                return;
        }
        Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
        obj->set("user_public_key", p->public_key);
        obj->set("device_id", Poco::DigestEngine::digestToHex(p->device_id));
        obj->set("address", p->lp->address());
        Poco::JSON::Object::Ptr ptr = new Poco::JSON::Object();
        std::string evnt = metax::connected == cmd ? "peer_is_connected" :
                "peer_is_disconnected";
        ptr->set("event", evnt);
        ptr->set("peer", obj);
        std::ostringstream oss;
        ptr->stringify(oss);
        router_link_package& out = *router_tx;
        out.cmd = cmd;
        out.set_payload(oss.str());
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_verify_peer_request(ID32 pid, platform::default_package di,
                                                const std::string& pbk)
{
        METAX_TRACE(__FUNCTION__);
        link_cfg_package& out = *config_tx;
        out.cmd = metax::verify_peer;
        out.peer_id = pid;
        out.device_id = di;
        out.set_payload(pbk);
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
accept_connection_with_balancing(leviathan::metax::link::peer_io* p,
                                                platform::default_package di)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        poco_assert(nullptr != p->lp);
        Poco::Mutex::ScopedLock block(m_balancer_mutex);
        if (-1 != p->lp->m_gid) {
                auto i = m_balance_info.find(p->lp->m_gid);
                if (m_balance_info.end() != i) {
                        auto j = peers.find(i->second);
                        poco_assert(peers.end() != j);
                        poco_assert(nullptr != j->second);
                        if (j->second->load_info <= p->load_info) {
                                METAX_INFO("balancer dropped connection with: "
                                                + p->lp->address());
                                send_shutdown_message_to_peer(p);
                                return;
                        }
                        METAX_INFO("balancer dropped connection with: "
                                        + j->second->lp->address());
                        send_shutdown_message_to_peer(j->second);
                }
                m_balance_info[p->lp->m_gid] = p->lp->m_id;
        }
        METAX_INFO("balancer accepted connection with: " + p->lp->address());
        METAX_INFO("Peers count is : " +
                        platform::utils::to_string(peers.size()));
        p->device_id.assign(di.message.get(), di.message.get() + di.size);
        std::string s = Poco::DigestEngine::digestToHex(p->device_id);
        configure_peer_by_device_id(p, s);
        send_notification_to_router(p, metax::connected);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_link_peer_device_id(leviathan::metax::link::peer_io* p,
                link_data& ld, uint32_t offset)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p); poco_assert(nullptr != p->lp);
        const link_peer_package& in = *(p->in);
        poco_assert(in.flags == platform::default_package::LINK_MSG);
        platform::default_package pkg;
        pkg.extract_payload(in, offset);
        std::string a = p->lp->address();
        if (nullptr == pkg.message.get()) {
                METAX_WARNING("Invalid device id is received from: " + a);
                return send_shutdown_message_to_peer(p);
        }
        METAX_DEBUG("Received " + Poco::DigestEngine::digestToHex(
                        Poco::DigestEngine::Digest(
                        pkg.message.get(), pkg.message.get() + pkg.size))
                        + " device id from: " + a);
        p->public_key = ld.public_key;
        METAX_DEBUG("Received " + ld.public_key + " public key from: " + a);
        p->load_info = ld.load_info;
        if (verification_mode::strict == m_peer_verification_mode) {
                send_verify_peer_request(p->lp->m_id, pkg, p->public_key);
        } else {
                poco_assert(
                        verification_mode::none == m_peer_verification_mode ||
                        verification_mode::relaxed == m_peer_verification_mode);
                accept_connection_with_balancing(p, pkg);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_link_message(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        const link_peer_package& in = *(p->in);
        poco_assert(0 != in.message);
        poco_assert(in.flags == platform::default_package::LINK_MSG);
        link_data ld;
        uint32_t offset = ld.extract_header(in);
        switch (ld.cmd) {
                case metax::device_id: {
                        handle_link_peer_device_id(p, ld, offset);
                        break;
                } default: {
                        METAX_WARNING("Receive unknown message for link "
                                        "from peer - " + p->lp->address());
                        send_shutdown_message_to_peer(p);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_router_message_of_link_peer(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        if (p->device_id.empty()) {
                poco_assert(nullptr != p->lp);
                METAX_WARNING("Received message from announced"
                                " peer - " + p->lp->address());
                send_shutdown_message_to_peer(p);
        } else {
                post_received_data_to_router(p);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
post_received_data_to_router(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        const link_peer_package& in = *(p->in);
        poco_assert(0 != in.message);
        METAX_DEBUG("Data message");
        router_link_package& out = (*router_tx);
        out = in;
        out.cmd = metax::received_data;
        poco_assert(nullptr != p->lp);
        out.peers.insert(p->lp->m_id);
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_peer_input(leviathan::metax::link::peer_io* p)
{
        if (! p->in.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_peer_input(p);
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        p->in.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
process_peer_input(leviathan::metax::link::peer_io* p) 
{
        poco_assert(p->in.has_data());
        METAX_TRACE(__FUNCTION__);
        METAX_DEBUG("Received message from  " + p->lp->address());
        const link_peer_package& in = *(p->in);
        switch(in.cmd) {
                case metax::data : {
                        handle_link_peer_data(p);
                        break;
                } case metax::disconnect : {
                        METAX_DEBUG("Disconnect request");
                        (p->out).deactivate();
                        (p->ping_tx).deactivate();
                        break;
                } default : {
                        std::stringstream ss;
                        ss << "Command is not handled yet : " << in.cmd;
                        METAX_WARNING(ss.str());
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
post_nack_to_router()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        *router_tx = *router_rx;
        (*router_tx).cmd = metax::nack;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::link::
is_rating_enough(unsigned int r, const peer_io* io)
{
        return r <= io->rating;
}

void leviathan::metax::link::
broadcast()
{
        METAX_TRACE(__FUNCTION__);
        bool is_broadcasted = false;
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        const router_link_package& in = *router_rx;
        for (auto i : peers) {
                poco_assert(nullptr != i.second);
                peer_io* io = i.second;
                if (! io->device_id.empty()
                                && is_rating_enough(in.cmd_rating, io)
                                && (! in.should_block_by_sync
                                        || i.second->lp->is_sync_finished)) {
                        METAX_DEBUG("Broadcasting message to  "
                                        + i.second->lp->address());
                        (*(io->out)).cmd = metax::data;
                        *(io->out) = in;
                        (io->out).commit();
                        is_broadcasted = true;
                }
        }
        if (! is_broadcasted) {
                post_nack_to_router();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
broadcast_with_exclude(const std::set<ID32>& p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        if (peers.empty()) {
                post_nack_to_router();
                return;
        }
        const router_link_package& in = *router_rx;
        for (auto i : peers) {
                poco_assert(nullptr != i.second);
                peer_io* io = i.second;
                if (! io->device_id.empty()
                                && is_rating_enough(in.cmd_rating, io)
                                && p.end() == p.find(i.first)
                                && (! in.should_block_by_sync
                                        || io->lp->is_sync_finished)) {
                        METAX_DEBUG("Broadcasting (with exclusion) message to "
                                        + io->lp->address());
                        (*(io->out)).cmd = metax::data;
                        *(io->out) = in;
                        (io->out).commit();
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
stop_reactor()
{
        METAX_TRACE(__FUNCTION__);
        METAX_INFO("Stopping reactor thread");
        m_reactor.stop();
        m_reactor.wakeUp();
        m_reactor_thread.join();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
stop_all_peers()
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto p : peers) {
                METAX_DEBUG("Stopping peer " + p.second->lp->address());
                p.second->lp->stop();
                update_peer_last_seen(p.second);
                METAX_DEBUG("End stopping peer " + p.second->lp->address());
                delete p.second;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
cleanup_link_peers()
{
        METAX_TRACE(__FUNCTION__);
        METAX_INFO("Stopping all peers");
        stop_all_peers();
        clean_peer_info();
        METAX_INFO("Waiting for all peers to finish");
        for (auto& p : peer_threads) {
                poco_assert(nullptr != p.second.first);
                METAX_DEBUG("Joining peer " + p.second.second);
                p.second.first->join();
                METAX_DEBUG("Deleting thread " + p.second.second);
                delete p.first;
                delete p.second.first;
        }
        peers.clear();
        peer_threads.clear();
        m_my_dropped_connections.clear();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
post_router_feedback(const std::vector<ID32> p)
{
        METAX_TRACE(__FUNCTION__);
        if (p.empty()) {
                return;
        }
        poco_assert(router_rx.has_data());
        (*router_tx).request_id = (*router_rx).request_id;
        (*router_tx) = static_cast<platform::default_package>(*router_rx);
        (*router_tx).cmd = metax::nack;
        (*router_tx).peers.insert(p.begin(), p.end());
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send(const std::set<ID32>& p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        std::vector<ID32> unreachable_peers;
        for (auto i : p) {
                link_peers::iterator peer = peers.find(i);
                if (peers.end() != peer) {
                        poco_assert(nullptr != peer->second);
                        peer_io* io = peer->second;
                        if (! io->device_id.empty()) {
                                METAX_DEBUG("Sending message to  "
                                                        + io->lp->address());
                                (*(io->out)).cmd = metax::data;
                                *(io->out) = *router_rx;
                                (io->out).commit();
                        }
                } else {
                        unreachable_peers.push_back(i);
                }
        }
        post_router_feedback(unreachable_peers);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
process_router_sync_finished_request(const std::set<ID32>& p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto i : p) {
                link_peers::iterator peer = peers.find(i);
                if (peers.end() != peer) {
                        poco_assert(nullptr != peer->second);
                        peer_io* p_io = peer->second;
                        poco_assert(nullptr != p_io->lp);
                        p_io->lp->is_sync_finished = true;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
process_router_sync_started_request(const std::set<ID32>& p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto i : p) {
                link_peers::iterator peer = peers.find(i);
                if (peers.end() != peer) {
                        poco_assert(nullptr != peer->second);
                        peer_io* p_io = peer->second;
                        poco_assert(nullptr != p_io->lp);
                        p_io->lp->is_sync_finished = false;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_router_input()
{
        if (! router_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_router_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        router_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
process_router_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const router_link_package& in = *router_rx;
        switch (in.cmd) {
                case metax::send: {
                        send(in.peers);
                        break;
                } case metax::broadcast: {
                        broadcast();
                        break;
                } case metax::broadcast_except: {
                        broadcast_with_exclude(in.peers);
                        break;
                } case metax::sync_finished: {
                        process_router_sync_finished_request(in.peers);
                        break;
                } case metax::sync_started: {
                        process_router_sync_started_request(in.peers);
                        break;
                } default : {
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_inputs_from_peers()
{
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto p : peers) {
                handle_peer_input(p.second);
        }
}

void leviathan::metax::link::
wait_for_input_channels()
{
        METAX_TRACE(__FUNCTION__);
        while (true) {
                METAX_INFO("Waiting for data");
                if (! wait()) {
                        break;
                }
                METAX_INFO("LINK Handle Data");
                handle_inputs_from_peers();
                handle_router_input();
                handle_config_input();
                handle_keep_alive_messages();
                handle_key_transfer_notification();
        }
        finish();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
add_peer(leviathan::metax::link_peer* l, Poco::Thread* t)
{
        METAX_TRACE(__FUNCTION__);
        create_new_peer_info(l);
        Poco::Mutex::ScopedLock rlock(m_removable_peers_mutex);
        peer_threads[l] = std::make_pair(t, l->address());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
remove_peer(leviathan::metax::link_peer* l)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock plock(m_peers_mutex);
        poco_assert(nullptr != l);
        ID32 pid = l->m_id;
        poco_assert(peers.end() != peers.find(pid));
        peer_io* pio = peers[pid];
        poco_assert(nullptr != pio);
        remove_from_balance_info(pio);
        Poco::Mutex::ScopedLock rlock(m_removable_peers_mutex);
        Poco::Thread* t = peer_threads[l].first;
        m_removable_peers.push_back(std::make_pair(l, t));
        peer_threads.erase(l);
        Poco::Mutex::ScopedLock ilock(m_input_mutex);
        update_peer_last_seen(pio);
        send_notification_to_router(pio, metax::disconnected);
        delete pio;
        peers.erase(pid);
        METAX_INFO("Peers count is : " +
                        platform::utils::to_string(peers.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
remove_from_balance_info(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_balancer_mutex);
        poco_assert(nullptr != p);
        poco_assert(nullptr != p->lp);
        auto i = m_balance_info.find(p->lp->m_gid);
        if (i->second == p->lp->m_id) {
                m_balance_info.erase(i);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
update_peer_last_seen(leviathan::metax::link::peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        poco_assert(nullptr != p->lp);
        std::string d = Poco::DigestEngine::digestToHex(p->device_id);
        if (d.empty() || ! p->lp->is_sync_finished) {
                return;
        }
        poco_assert(nullptr != m_peers_rating);
        poco_assert(! m_peers_rating.isNull());
        if (! m_peers_rating->has(d)) {
                Poco::JSON::Object::Ptr n = new Poco::JSON::Object;
                m_peers_rating->set(d, n);
        }
        Poco::JSON::Object::Ptr pi = m_peers_rating->getObject(d);
        poco_assert(! pi.isNull());
        pi->set("last_seen", p->last_received_time.epochMicroseconds());
        save_peers_rating_in_file();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
save_peers_rating_in_file()
{
        METAX_TRACE(__FUNCTION__);
        std::ofstream ofs(m_peers_rating_path);
        if (! ofs.is_open()) {
                METAX_ERROR("Failed to update peer last seen time." );
                return;
        }
        // consider writing in tmp and then mv
        m_peers_rating->stringify(ofs);
        ofs.close();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
mark_as_disconnected(leviathan::metax::link_peer* l, Poco::Thread* t)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_removable_peers_mutex);
        m_removable_peers.push_back(std::make_pair(l, t));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_dropped_connections()
{
        METAX_TRACE(__FUNCTION__);
        clean_peer_info();
        reconnect_my_dropped_connections();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
on_timer(Poco::Timer&)
{
        handle_dropped_connections();
}

void leviathan::metax::link::
create_new_peer_info(leviathan::metax::link_peer* l)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        Poco::Mutex::ScopedLock ilock(m_input_mutex);
        ID32 pid = get_peer_id();
        l->m_id = pid;
        peer_io* p = new peer_io(this);
        p->lp = l;
        p->out.connect(l->link_rx);
        l->link_tx.connect(p->in);
        peers[pid] = p;
        p->ping_tx.connect(l->ping_rx);
        METAX_INFO("Adding new peer... : " + l->address());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
clean_peer_info()
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock dlock(m_dropped_connections_mutex);
        Poco::Mutex::ScopedLock rlock(m_removable_peers_mutex);
        auto i = m_removable_peers.begin();
        auto e = m_removable_peers.end();
        while (i != e) {
                link_peer* l = i->first;
                Poco::Thread* t = i->second;
                poco_assert(nullptr != l);
                METAX_INFO("waiting peer to finish: " + l->address());
                poco_assert(nullptr != t);
                t->join();
                if (l->is_my_connection()) {
                        m_my_dropped_connections.push_back(
                                        std::make_pair(l->address(), l->m_gid));
                }
                METAX_INFO("deleting peer: " + l->address());
                delete l;
                delete t;
                i = m_removable_peers.erase(i);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
reconnect_my_dropped_connections()
{
        METAX_TRACE(__FUNCTION__);
        auto i = m_my_dropped_connections.begin();
        auto e = m_my_dropped_connections.end();
        Poco::Mutex::ScopedLock lock(m_balancer_mutex);
        while (i != e) {
                if (m_balance_info.end() == m_balance_info.find(i->second)) {
                        new link_peer(i->first, i->second, m_reactor);
                        i = m_my_dropped_connections.erase(i);
                } else {
                        ++i;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_shutdown_message_to_peer(peer_io* p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != p);
        poco_assert(nullptr != p->lp);
        METAX_WARNING(p->lp->address() + " peer is offline");
        p->device_id.clear();
        (*(p->out)).cmd = metax::shutdown;
        (*(p->out)).set_payload(std::string("Shutdown peer"));
        (p->out).commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
send_notification_to_peer(const std::string& m)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
        poco_assert(nullptr != p);
        p->set("KeyTransfer", m);
        std::stringstream ss;
        p->stringify(ss);
        link_cfg_package& out = *config_tx;
        out.cmd = metax::send_to_peer;
        out.set_payload(ss.str());
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_key_transfer_notification()
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock l(m_notification_mutex);
        if (! m_notification.empty()) {
               send_notification_to_peer(m_notification); 
               m_notification = "";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
handle_keep_alive_messages() 
{
        Poco::Mutex::ScopedLock l(m_keep_alive_timeout_mutex);
        if (! m_keep_alive_timeout) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_peers_mutex);
        for (auto i : peers) {
                peer_io* io = i.second;
                poco_assert(nullptr != io);
                if (io->ping_sent) {
                        if (io->last_received_time.isElapsed(PONG_WAIT_TIME)) {
                                METAX_INFO(
                                        "no response from Peer, shutting down: "
                                        + io->lp->address());
                                send_shutdown_message_to_peer(io);
                        } else {
                                io->ping_sent = false;
                        }
                } else {
                        if (io->last_received_time.isElapsed(PONG_WAIT_TIME)) {
                                if (io->device_id.empty()) {
                                        METAX_INFO("no device id is received "
                                            "for a while, disconnect from peer: "
                                            + io->lp->address());
                                        send_shutdown_message_to_peer(io);
                                } else {
                                        METAX_INFO("no activity from peer for "
                                                "a while, sending ping message: "
                                                + io->lp->address());
                                        io->ping_sent = true;
                                        platform::default_package pkg;
                                        pkg.flags =
                                                platform::default_package::PING;
                                        *(io->ping_tx) = pkg;
                                        (io->ping_tx).commit();
                                }
                        }
                }
        }
        m_keep_alive_timeout = false;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
on_keep_timer(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_keep_alive_timeout_mutex);
        m_keep_alive_timeout = true;
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::
runTask ()
try {
        METAX_TRACE(__FUNCTION__);
        configure();
        wait_for_input_channels();
        METAX_INFO("Stopping connection guard timer");
        m_connection_guard.stop();
        METAX_INFO("Stopping keep alive timer");
        m_keep_alive.stop();
        if (nullptr != m_acceptor) {
                METAX_INFO("unregistering socket connection Acceptor");
                m_acceptor->unregisterAcceptor();
        }
        if (nullptr != m_secure_acceptor) {
                METAX_INFO("unregistering secure socket connection Acceptor");
                m_secure_acceptor->unregisterAcceptor();
        }
        METAX_INFO("Stopping peers");
        cleanup_link_peers();
        stop_reactor();
        if (nullptr != m_server_socket) {
                METAX_INFO("Closing server socket");
                m_server_socket->close();
        }
        cancel_pairing();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch(const Poco::Exception& e) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL(e.displayText());
        std::terminate();
} catch(...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhadled exception.");
        std::terminate();
}

leviathan::metax::link::
link()
        : platform::csp::task("link", Poco::Logger::get("metax.link"))
        , router_rx(this)
        , router_tx(this)
        , config_rx(this)
        , config_tx(this)
        , key_manager_rx(this)
        , key_manager_tx(this)
        , peers()
        , device_id()
        , public_key()
        , use_ssl(true)
        , m_request_counter(LINK_REQUEST_LOWER_BOUND)
        , m_reactor(this)
        , m_server_socket(nullptr)
        , m_acceptor(nullptr)
        , m_secure_acceptor(nullptr)
        , CONNECTION_GUARD_TIMEOUT(5000) // milliseconds
        , m_connection_guard(60000, CONNECTION_GUARD_TIMEOUT)
        , m_on_timer_callback(*this, &link::on_timer)
        , m_dropped_connections_mutex()
        , m_my_dropped_connections()
        , m_removable_peers()
        , KEEP_ALIVE_TIMEOUT(30000) // milliseconds
        , PONG_WAIT_TIME((KEEP_ALIVE_TIMEOUT + 1) * 1000) // microseconds
        , m_keep_alive(KEEP_ALIVE_TIMEOUT, KEEP_ALIVE_TIMEOUT)
        , m_on_keep_alive_call(*this, &link::on_keep_timer)
        , m_keep_alive_timeout_mutex()
        , m_keep_alive_timeout(false)
        , m_peers_rating_path()
        , m_peers_rating(new Poco::JSON::Object)
        , m_peer_verification_mode(verification_mode::none)
        , m_balance_info()
        , m_pairing_port(9001)
        , m_user_key_path("")
        , m_user_json_info_path("")
        , m_device_key_path("")
        , m_prkey_filename("private.pem")
        , m_pbkey_filename("public.pem")
        , m_cert_filename("cert.pem")
        , m_pairing_socket(nullptr)
        , m_pairing_timout(0)
        , m_generated_code("")
        , m_notification_mutex()
        , m_notification("")
        , m_start_pairing_timer(0, 0)
{
        METAX_TRACE(__FUNCTION__);
        METAX_INFO("Metax Link layer is constructed");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::link::
~link()
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr != m_acceptor) {
                delete m_acceptor;
                m_acceptor = nullptr;
        }
        if (nullptr != m_secure_acceptor) {
                delete m_secure_acceptor;
                m_secure_acceptor = nullptr;
        } 
        if (nullptr != m_server_socket) {
                delete m_server_socket;
                m_server_socket = nullptr;
        } 
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::link::newConnection::
newConnection(const Poco::Net::StreamSocket& s, link* l)
        : Poco::Net::TCPServerConnection(s)
        , m_link(l)
        , m_logger(Poco::Logger::get("metax.link"))
{
        poco_assert(nullptr != m_link);
}

void leviathan::metax::link::newConnection::
send_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_link);
        poco_assert(! m_link->m_user_key_path.empty());
        key_transfer_package ktp;
        ktp.public_key = platform::utils::read_file_content(
                        m_link->m_user_key_path + "/public.pem", false);
        poco_assert(! ktp.public_key.empty());
        ktp.private_key = platform::utils::read_file_content(
                        m_link->m_user_key_path + "/private.pem", false);
        poco_assert(! ktp.private_key.empty());
        ktp.user_json_info = platform::utils::read_file_content(
                        m_link->m_user_json_info_path, false);
        poco_assert(! ktp.user_json_info.empty());
        platform::default_package m;
        ktp.serialize(m);
        link_peer::send_by_socket(socket(), m);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::link::newConnection::
run()
try {
        METAX_TRACE(__FUNCTION__);
        std::string s = socket().peerAddress().host().toString();
        METAX_INFO(std::string("Transfer keys: New connection from ") + s);
        key_transfer_package hm;
        hm.set_payload(std::string("Connected"));
        platform::default_package hp;
        hm.serialize(hp);
        link_peer::send_by_socket(socket(), hp);
        METAX_INFO(std::string("Transfer keys: Completed handshake ") + s);
        poco_assert(nullptr != m_link);
        if (m_link->m_generated_code.empty()) {
                return;
        }
        platform::default_package pkg = link_peer::read_message(socket());
        key_transfer_package ktp;
        ktp.extract_header(pkg);
        if (ktp.code == m_link->m_generated_code) {
                send_user_keys();
                Poco::Mutex::ScopedLock lock(m_link->m_notification_mutex);
                m_link->m_notification = "Successfully sent keys to " + s;
                m_link->wake_up();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING("Error in pairing socket: " + e.displayText());
}

leviathan::metax::link::newConnectionFactory::
newConnectionFactory(link* l)
        : Poco::Net::TCPServerConnectionFactory()
        , m_link(l)
{
        poco_assert(nullptr != m_link);
}

Poco::Net::TCPServerConnection* leviathan::metax::link::newConnectionFactory::
createConnection(const Poco::Net::StreamSocket& socket)
{
        poco_assert(nullptr != m_link);
        newConnection* n = new newConnection(socket, m_link);
        return n;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

