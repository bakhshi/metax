/**
 * @file src/metax/configuration_manager.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::configuration_manager
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_CONFIGURATION_MANAGER_HPP
#define LEVIATHAN_METAX_CONFIGURATION_MANAGER_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/JSON/Object.h>
#include <Poco/Path.h>
#include <Poco/MD5Engine.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct configuration_manager;
        }
}

namespace Poco {
        namespace Util {
                class XMLConfiguration;
        }
}


/**
 * @brief Parses and keeps configurations from config.xml and configures all
 * other tasks, processes all the requests concerning Metax configuration, e.g.
 * number of peers, getting public key, etc.
 *
 */
struct leviathan::metax::configuration_manager:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        using INPUT = platform::csp::input<platform::default_package>;
        using STORAGE_RX = platform::csp::input<storage_cfg_package>;
        using KERNEL_RX = platform::csp::input<metax::kernel_cfg_package>;
        using ROUTER_RX = platform::csp::input<metax::router_cfg_package>;

        using OUTPUT = platform::csp::output<platform::default_package>;
        using STORAGE_TX = platform::csp::output<storage_cfg_package>;
        using KERNEL_TX = platform::csp::output<metax::kernel_cfg_package>;
        using ROUTER_TX = platform::csp::output<metax::router_cfg_package>;

        using KEY_MANAGER_RX = leviathan::platform::csp::input<key_manager_cfg_package>;
        using KEY_MANAGER_TX = leviathan::platform::csp::output<key_manager_cfg_package>;

        using BACKUP_RX = platform::csp::input<backup_cfg_package>;
        using BACKUP_TX = platform::csp::output<backup_cfg_package>;

        struct data {
                std::string cfg_path;
                /**
                 * stores peer address in string as host:port and group id,
                 * which are used to load balance by keeping only one connection
                 * from the same group
                 */
                std::vector<std::pair<std::string, int>> peers;
                int port;
                bool use_ssl;
                std::string device_key_path;
                std::string user_key_path;
                std::string storage_path;
                std::string trusted_peers_json;
                std::string friends_json;
                double cache_size_limit_in_gb; // in GB
                double size_limit_in_gb; // in GB
                /** Files in cache older than specified time will be removed.
                 * The time should be specified in minutes. It can be
                 * fractional, e.g. 0.5 means 30 seconds.
                 */
                double cache_age;
                unsigned int storage_class;
                unsigned int sync_class;
                bool enable_livestream_hosting;
                uint16_t data_copy_count;
                uint16_t metadata_dump_timer;
                uint32_t router_save_queue_size;
                uint32_t router_get_queue_size;
                std::string device_json;
                std::string user_json_info;
                std::string metax_user_info_path;
                std::string metax_device_info_path;

                /**
                 * Path of peers rating configuration file. It should contain
                 * rating information about peers by the following format:
                 * {
                 *      "device_id1": <integer>,
                 *      "device_id2": <integer>,
                 *      ...
                 * }
                 */
                std::string peers_rating;

                /**
                 * Maximum hop count of tender. Should be positive integer
                 * grater than 0.
                 */
                Poco::UInt16 max_hops;

                Poco::UInt16 peer_response_wait_time;

                /**
                 * Default value of peers rating. This value is used for the
                 * peers which rating is not specified in peers rating
                 * configuration file. Should be integer from 0-100 range.
                 */
                Poco::UInt16 peers_default_rating;

                /**
                 * Defines the version of configuration file structure.
                 */
                unsigned int version;

                std::string peer_verification_mode;

                data()
                        : port(7070)
                        , use_ssl(true)
                        , device_key_path(Poco::Path::home() +
                                        ".leviathan/keys/device")
                        , user_key_path(Poco::Path::home() +
                                        ".leviathan/keys/user")
                        , storage_path(Poco::Path::home() +
                                        ".leviathan/storage")
                        , trusted_peers_json(Poco::Path::home() +
                                        ".leviathan/trusted_peers.json")
                        , friends_json(Poco::Path::home() +
                                        ".leviathan/friends.json")
                        , cache_size_limit_in_gb(0)
                        , size_limit_in_gb(-1)
                        , cache_age(2)
                        , storage_class(2)
                        , sync_class(0)
                        , enable_livestream_hosting(false)
                        , data_copy_count(4)
                        , metadata_dump_timer(5)
                        , router_save_queue_size(21000000)
                        , router_get_queue_size(21000000)
                        , device_json(Poco::Path::home() +
                                        ".leviathan/device.json")
                        , user_json_info(Poco::Path::home() +
                                        ".leviathan/keys/user_json_info.json")
                        , metax_user_info_path(Poco::Path::home() +
                                        ".leviathan/metax_user_info.json")
                        , metax_device_info_path(Poco::Path::home() +
                                        ".leviathan/metax_device_info.json")
                        , peers_rating(Poco::Path::home() +
                                        ".leviathan/peers_rating.json")
                        , max_hops(3)
                        , peer_response_wait_time(30)
                        , peers_default_rating(100)
                        , version(1)
                        , peer_verification_mode("none")

                {}
        };

        struct peer_info {
                std::string device_key;
                std::string user_key;

                peer_info(const std::string& d = "", const std::string& u = "")
                        : device_key(d)
                        , user_key(u)
                {}
        };

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

        /// @name Helper functions
private:
        void handle_storage_input();
        void handle_cache_input();
        void process_storage_input(const storage_cfg_package& in);
        void handle_wallet_input();
        void process_wallet_input();
        void handle_router_input();
        void process_router_input();
        void handle_link_input();
        void process_link_input();
        void handle_key_manager_input();
        void process_key_manager_input();
        void handle_kernel_input();
        void process_kernel_input();
        /**
         * @brief Receives "username" from kernel,
         * and finds its public key from m_trusted_peers map.
         */
        void handle_kernel_add_trusted_peer();
        void handle_kernel_remove_trusted_peer();
        void handle_kernel_get_trusted_peer_list();
        void handle_kernel_add_friend();
        void handle_kernel_get_friend_list();
        void handle_kernel_get_friend();
        void handle_kernel_get_public_key();
        void handle_kernel_get_online_peers();
        void handle_kernel_get_pairing_peers();
        void handle_kernel_get_metax_info();
        void handle_kernel_set_metax_info();
        void handle_kernel_reconnect_to_peers();
        void handle_kernel_start_pairing();
        void handle_kernel_cancel_pairing();
        void handle_kernel_request_keys();
        void send_friend_not_found();
        void parse_config(const std::string&);
        void expand_file_channels_path(Poco::Util::XMLConfiguration* c);
        void configure_logger(Poco::Util::XMLConfiguration* c);
        void add_peer_address(const Poco::Util::XMLConfiguration* c,
                        const std::string& pr, const std::string& ad, int g);
        void get_peer_info(const Poco::Util::XMLConfiguration*);
        void get_device_key_path(const Poco::Util::XMLConfiguration* c);
        void get_user_key_path(const Poco::Util::XMLConfiguration* c);
        void get_storage_config(const Poco::Util::XMLConfiguration* c);
        void get_storage_class(const Poco::Util::XMLConfiguration* c);
        void get_sync_class(const Poco::Util::XMLConfiguration* c);
        void get_enable_livestream_hosting(
                                const Poco::Util::XMLConfiguration* c);
        void get_data_copy_count(const Poco::Util::XMLConfiguration* c);
        void get_metadata_dump_timer(const Poco::Util::XMLConfiguration* c);
        void get_router_save_queue_size(const Poco::Util::XMLConfiguration* c);
        void get_router_get_queue_size(const Poco::Util::XMLConfiguration* c);
        void get_trusted_peers_json_path(const Poco::Util::XMLConfiguration* c);
        void get_friends_json_path(const Poco::Util::XMLConfiguration* c);
        void get_device_json_path(const Poco::Util::XMLConfiguration* c);
        void get_user_json_info_path(const Poco::Util::XMLConfiguration* c);
        void get_config_version(const Poco::Util::XMLConfiguration* c);
        void get_peer_verification_mode(const Poco::Util::XMLConfiguration* c);

        void initialize_user_manager();
        void initialize_link_layer();
        void initialize_key_manager();
        void initialize_storage();
        void initialize_backup();
        void initialize_kernel();
        void initialize_router();
        void handle_key_manager_initialized_keys();
        void set_storage_configuration(storage_cfg_package& out);
        void write_storage_configuration(const metax::storage_cfg_package& in);
        void get_peers_ratings(const Poco::Util::XMLConfiguration* c);
        void get_peers_default_rating(const Poco::Util::XMLConfiguration* c);
        void get_max_hops(const Poco::Util::XMLConfiguration* c);
        void get_peer_response_wait_time(const Poco::Util::XMLConfiguration* c);
        void get_metax_info_path(const Poco::Util::XMLConfiguration* c);
        void handle_link_send_notification_to_peer();
        void handle_link_get_online_peers_response();
        void handle_link_get_pairing_peers_response();
        void handle_link_get_generated_code();
        void handle_link_start_pairing_fail();
        void handle_link_request_keys_fail();
        void send_add_peer_failed_to_kernel(ID32 rid, const std::string& n,
                        const std::string err);
        void handle_link_request_keys_response();
        void handle_link_verify_peer_request();
        void send_peer_verify_response(ID32 pid,
                        platform::default_package di, metax::command cmd);
        void send_add_peer_failed_to_kernel(ID32 rid, const std::string err);
        void send_remove_peer_failed_to_kernel(ID32 rid, const std::string err);
        void send_add_friend_failed_to_kernel(ID32 rid, const std::string& n,
                const std::string err);
        void send_add_peer_confirm_to_kernel(ID32 rid);
        void send_remove_peer_confirm_to_kernel(ID32 rid);
        void send_add_friend_confirm_to_kernel(ID32 rid, const std::string& n);
        std::string get_trusted_peer_list();
        std::string get_friend_list();
        void handle_storage_writer_input();

public:
        KEY_MANAGER_RX key_manager_rx;
        STORAGE_RX storage_rx;
        STORAGE_RX storage_writer_rx;
        STORAGE_RX cache_rx;
        INPUT wallet_rx;
        leviathan::platform::csp::input<link_cfg_package> link_rx;
        ROUTER_RX router_rx;
        KERNEL_RX kernel_rx;
        BACKUP_RX backup_rx;

        KEY_MANAGER_TX key_manager_tx;
        STORAGE_TX storage_tx;
        STORAGE_TX storage_writer_tx;
        STORAGE_TX cache_tx;
        OUTPUT wallet_tx;
        leviathan::platform::csp::output<link_cfg_package> link_tx;
        ROUTER_TX router_tx;
        KERNEL_TX kernel_tx;
        BACKUP_TX backup_tx;
        BACKUP_TX user_manager_tx;

private:
        data cfg_data;
        std::string m_base_path;
        /**
         * @brief Peer's username maps to its public key.
         */
        std::map<Poco::DigestEngine::Digest, peer_info> m_trusted_peers;

        /**
         * @brief Friend's username maps to its public key.
         */
        std::map<std::string, std::string> m_friends;

        /**
         * @brief Engine to compute md5
         */
        Poco::MD5Engine m_md5_engine;

        /// @name Special member functions
public:
        /**
         * @brief Default constructor
         *
         * @param cfg_path - path to configuration file
         * @param base_path - there are paths in config file, like storage
         * path, public/private key files, etc. If those paths are not absolute
         * paths then base_path as appended at the beginnings of those paths.
         *
         */
        configuration_manager(const std::string& cfg_path = "config.xml",
                        const std::string& base_path = "");

        /// Destructor
        virtual ~configuration_manager();

        /// @name Disabled special member functions
private:
        void configure_root_logger();
        /// This class is not copy-constructible
        configuration_manager(const configuration_manager&);

        /// This class is not assignable
        configuration_manager& operator=(const configuration_manager&);

        /// This class is not copy-constructible
        configuration_manager(const configuration_manager&&);

        /// This class is not assignable
        configuration_manager& operator=(const configuration_manager&&);

}; // class leviathan::metax::configuration_manager


#endif // LEVIATHAN_METAX_CONFIGURATION_MANAGER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

