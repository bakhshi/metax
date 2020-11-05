/**
 * @file src/metax/link.hpp
 *
 * @brief Definition of the class @ref
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

#ifndef LEVIATHAN_METAX_LINK_HPP
#define LEVIATHAN_METAX_LINK_HPP


// Headers from this project
#include "socket_reactor.hpp"
#include "link_peer.hpp"
#include "protocols.hpp"
#include "socket_acceptor.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/TCPServer.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/SocketAcceptor.h>
#include <Poco/Timer.h>
#include <Poco/DigestEngine.h>
#include <Poco/Net/PrivateKeyFactory.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>

// Headers from standard libraries
#include <set>
#include <string>
#include <atomic>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct link;
                struct link_peer;
        }
}

/**
 * @brief Manages peer connections.
 * 
 */
struct leviathan::metax::link
        : leviathan::platform::csp::task
{
        /// @name Type definitions
public:
       struct peer_io {
               platform::csp::input<link_peer_package> in;
               platform::csp::output<link_peer_package> out;
               platform::csp::output<link_peer_package> ping_tx;
               link_peer* lp;
               Poco::Timestamp last_received_time;
               bool ping_sent;
               Poco::DigestEngine::Digest device_id;
               int rating;
               std::string public_key;
               ID32 load_info;

               peer_io(link* l)
                       : in(l)
                       , out(l)
                       , ping_tx(l)
                       , last_received_time()
                       , ping_sent(false)
                       , device_id()
                       , rating(0)
                       , public_key()
                       , load_info(0)
               {
               }
       };

        enum class verification_mode {
                strict, relaxed, none
        };


        /// @name Starting the task
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

        /// @name Peer handling
public:
        void add_peer(leviathan::metax::link_peer* p,
                        Poco::Thread* t);
        void remove_peer(leviathan::metax::link_peer* p);

        /**
         * @brief  Stores disconnected peer address for connecting later
         *
         * @details If the connection request went from me and connection is
         * dropped, I should try to connect to the peer later. This function
         * stores the such peer information in a container and a timer job tries
         * to connect to peers from that container.
         */
        void mark_as_disconnected(leviathan::metax::link_peer* l,
                                                        Poco::Thread* t);
        void post_send_journal_info_from_last_seen(
                        platform::csp::output<link_peer_package>& out,
                        Poco::JSON::Object::Ptr pi);
        void post_kernel_start_storage_sync(leviathan::metax::link::peer_io* p,
                        Poco::JSON::Object::Ptr pi);
        Poco::UInt64 get_peer_last_seen(Poco::JSON::Object::Ptr pi) const;

        leviathan::metax::ID32 get_peer_id();

        /// @name Helper functions
private:
        void send_notification_to_peer(const std::string& m);

        /**
         * @brief We should run our own socket server in case of anyone would
         * like to establish direct connection with us.
         * Socket reactor is started to handle updates on sockets wrapped in
         * link_peer.
         *
         * @details Our server should listen on a port in a separate thread,
         * while in parallel we can communicate with other peers. The port
         * number on which the server is listening is obtained from
         * link configuration.
         */
        void configure();

        /**
         * @brief Waits for any message received from router, config or socket
         * peers and handles them
         */
        void wait_for_input_channels();

        void handle_config_input();
        void process_config_input();
        void handle_connect_request();
        void handle_bind_request();
        void handle_peer_input(link::peer_io* p);
        void process_peer_input(link::peer_io* p);
        void handle_inputs_from_peers();
        void handle_router_input();
        void process_router_input();
        void send(const std::set<ID32>&);

        /**
         * @brief  Posts nack message to router as a reply to broadcast
         * (broadcast_with_exclude) requests if no peer is available
         */
        void post_nack_to_router();

        void broadcast();
        void broadcast_with_exclude(const std::set<ID32>&);
        void post_router_feedback (const std::vector<ID32> p);


        /**
         * @brief  This is a timer callback to reconnect to dropped peers
         *
         * @details Wakes up and checks whether there are dropped peers to whith
         * whom I should undertake the reconnection. If the dropped connection
         * list is not empty tries to reconnect to peers and removes them from
         * the list in case of success.
         *
         */
        void on_timer(Poco::Timer&);
        void on_keep_timer(Poco::Timer&);
        void handle_dropped_connections();
        void post_received_data_to_router(leviathan::metax::link::peer_io* p);
        void handle_empty_data(leviathan::metax::link::peer_io* p);
        void handle_keep_alive_messages() ;
        void create_new_peer_info(leviathan::metax::link_peer* l);
        void clean_peer_info();
        void reconnect_my_dropped_connections();
        void send_shutdown_message_to_peer(peer_io* p);
        void stop_reactor();
        void stop_all_peers();
        void cleanup_link_peers();
        void handle_config_device_id_request();
        void handle_config_get_online_peers_request();
        void handle_config_reconnect_to_peers();
        void handle_link_peer_data(leviathan::metax::link::peer_io* p);
        void handle_link_message(leviathan::metax::link::peer_io* p);
        void send_verify_peer_request(ID32 pid,
                        platform::default_package di, const std::string& pbk);
        void accept_connection_with_balancing(
                                leviathan::metax::link::peer_io* p,
                                platform::default_package di);
        void handle_link_peer_device_id(leviathan::metax::link::peer_io* p,
                        link_data& ld,
                        uint32_t offset);
        void configure_peer_by_device_id(leviathan::metax::link::peer_io* p,
                        const std::string& s);
        void handle_router_message_of_link_peer(peer_io* p);
        void handle_device_id(const std::string& path);
        void handle_user_public_key(const std::string& path);
        void handle_peers_ratings(const std::string& path);
        void handle_user_keys(const std::string& path);
        void handle_user_json_info(const std::string& path);
        void handle_peer_verification_mode(const std::string& mode);
        void validate_peers_rating(const Poco::JSON::Object::Ptr& obj);
        bool is_rating_enough(unsigned int r, const peer_io* io);
        Poco::JSON::Array::Ptr get_online_peers_list();
        void update_peer_last_seen(leviathan::metax::link::peer_io* p);
        void process_router_sync_finished_request(const std::set<ID32>& p);
        void process_router_sync_started_request(const std::set<ID32>& p);
        void save_peers_rating_in_file();
        void remove_from_balance_info(leviathan::metax::link::peer_io* p);

        /// @name I/O
public:
        using ROUTER_RX = platform::csp::input<router_link_package>;
        using ROUTER_TX = platform::csp::output<router_link_package>;
        using CONFIG_RX = platform::csp::input<link_cfg_package>;
        using CONFIG_TX = platform::csp::output<link_cfg_package>;
        using KEY_MGR_RX = platform::csp::input<link_key_manager_package>;
        using KEY_MGR_TX = platform::csp::output<link_key_manager_package>;
        using link_peers = std::map<ID32, peer_io*>;
        using link_peer_threads =
                std::map<link_peer*, std::pair<Poco::Thread*, std::string>>;

        ROUTER_RX router_rx;
        ROUTER_TX router_tx;
        CONFIG_RX config_rx;
        CONFIG_TX config_tx;
        KEY_MGR_RX key_manager_rx;
        KEY_MGR_TX key_manager_tx;


        link_peers peers;
        link_peer_threads peer_threads;
        Poco::DigestEngine::Digest device_id;
        std::string public_key;
        bool use_ssl;

        /// @name Private data
private:
        /**
         * Peer id counter domain is defined in the following way:
         * [LINK_REQUEST_LOWER_BOUND, LINK_REQUEST_UPPER_BOUND]
         *
         * See definition of macros in protocols.hpp
         */
        std::atomic<metax::ID32> m_request_counter;
        socket_reactor m_reactor;
        Poco::Net::ServerSocket* m_server_socket;
        Poco::Thread m_reactor_thread;
        Poco::Net::SocketAcceptor<link_peer>* m_acceptor;
        socket_acceptor<link_peer>* m_secure_acceptor;
        Poco::Mutex m_peers_mutex;
        Poco::Mutex m_removable_peers_mutex;
        /**
         * This timer is used to reconnect to those dropped peers whith whome I
         * should undertake the connection.
         *
         */
        const Poco::Timestamp::TimeVal CONNECTION_GUARD_TIMEOUT;
        Poco::Timer m_connection_guard;
        Poco::TimerCallback<link> m_on_timer_callback;
        Poco::Mutex m_dropped_connections_mutex;
        std::list<std::pair<std::string, int>> m_my_dropped_connections;
        std::list<std::pair<link_peer*, Poco::Thread*>> m_removable_peers;

        /**
         * This timer is used to check the ack messages for sent messages.
         */
        const Poco::Timestamp::TimeVal KEEP_ALIVE_TIMEOUT;
        const Poco::Timestamp::TimeDiff PONG_WAIT_TIME;
        Poco::Timer m_keep_alive;
        Poco::TimerCallback<link> m_on_keep_alive_call;
        Poco::Mutex m_keep_alive_timeout_mutex;
        bool m_keep_alive_timeout;
        std::string m_peers_rating_path;
        Poco::JSON::Object::Ptr m_peers_rating;

        /**
         * Default value of peers rating which is used for the
         * peers which rating is not specified in peers rating
         * configuration file. Can be specified in configuration file.
         */
        Poco::UInt16 m_peers_default_rating;
        Poco::UInt16 m_sync_class;
        verification_mode m_peer_verification_mode;
        Poco::Mutex m_balancer_mutex;
        std::map<ID32, ID32> m_balance_info;


        ///@name Key transfer functionality
private:
        const uint16_t m_pairing_port;
        std::string m_user_key_path;
        std::string m_user_json_info_path;
        std::string m_device_key_path;

        /// Server side
private:
        class newConnection: public Poco::Net::TCPServerConnection {
        public:
                void run();

        private:
                void send_user_keys();
                std::string name()
                {
                        return "link.newConnection";
                }

        private:
                link* m_link;
                Poco::Logger& m_logger;

        public:
                newConnection(const Poco::Net::StreamSocket& s, link* l);
        };

        class newConnectionFactory:
                        public Poco::Net::TCPServerConnectionFactory {
        public:
                Poco::Net::TCPServerConnection* createConnection(
                        const Poco::Net::StreamSocket& socket);
        private:
                link* m_link;
        public:
                newConnectionFactory(link* l);
        };

        void save_start_pairing_request_info();
        void initialize_server_ssl_manager();
        void create_pairing_tcp_server_socket();
        std::string generate_code(uint8_t);
        void send_fail_message(const std::string& m, metax::command cmd);
        void send_generated_code();
        void on_expired_start_pairing(Poco::Timer&);
        void handle_config_start_pairing();
        void cancel_pairing();
        void handle_key_transfer_notification();
        void send_notification_to_router(peer_io* p, command cmd);

        const std::string m_prkey_filename;
        const std::string m_pbkey_filename;
        const std::string m_cert_filename;
        Poco::Net::TCPServer* m_pairing_socket;
        uint16_t m_pairing_timout;
        std::string m_generated_code;
        Poco::Mutex m_notification_mutex;
        std::string m_notification;
        Poco::Timer m_start_pairing_timer;

        /// Client side
private:
        void initialize_client_ssl_manager();
        uint32_t address_to_int(const std::string& s);
        std::string int_to_address(uint32_t n);
        void get_info_from_system(int fd, struct ifreq* ifr,
                        std::vector<std::pair<std::string, std::string>>& v);
        void get_local_ip_addresses_and_subnets(
                std::vector<std::pair<std::string, std::string>>& v);
        bool create_connection(const std::string&);
        void collect_available_ips_in_specified_network(const std::string& ip,
                const std::string& mask, std::set<std::string>& p);
        Poco::JSON::Array::Ptr get_pairing_peers_list();
        void handle_config_get_pairing_peers();
        std::string get_ip_from_request_keys_request();
        std::string get_code_from_request_keys_request();
        void save_user_keys(Poco::Net::SecureStreamSocket& client, ID32 req_id);
        void connect_server_and_save_keys(const std::string& current_address,
                const std::string& code, ID32 req_id);
        void handle_config_request_keys();
        void send_request_keys_fail_to_config(const std::string err,
                        ID32 req_id);
        void handle_config_peer_verified();
        void handle_config_peer_not_verified();

        /// @name Special member functions
public:
        /// Default constructor
        link();

        /// Destructor
        virtual ~link();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        link(const link&) = delete;

        /// This class is not assignable
        link& operator=(const link&) = delete;

}; // class leviathan::metax::link

#endif // LEVIATHAN_METAX_LINK_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

