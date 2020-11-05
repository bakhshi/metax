/**
 * @file src/metax/backup.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::backup
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

#ifndef LEVIATHAN_METAX_BACKUP_HPP
#define LEVIATHAN_METAX_BACKUP_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/StringTokenizer.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Timer.h>

// Headers from standard libraries
#include <fstream>
#include <map>
#include <functional>
#include <atomic>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct backup;
        }
}

/**
 * @brief Implements data backup logic.
 */
struct leviathan::metax::backup:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        using ROUTER_RX = platform::csp::input<kernel_router_package>;
        using ROUTER_TX = platform::csp::output<kernel_router_package>;

        using STORAGE_RX = platform::csp::input<kernel_storage_package>;
        using STORAGE_TX = platform::csp::output<kernel_storage_package>;

        using KERNEL_RX = platform::csp::input<kernel_backup_package>;
        using KERNEL_TX = platform::csp::output<kernel_backup_package>;

        using CONFIG_RX = platform::csp::input<backup_cfg_package>;
        using CONFIG_TX = platform::csp::output<backup_cfg_package>;

        using USER_MANAGER_RX =
                platform::csp::input<backup_user_manager_package>;
public:
        KERNEL_RX kernel_rx;
        ROUTER_RX router_rx;
        CONFIG_RX config_rx;
        STORAGE_RX storage_rx;
        STORAGE_RX storage_writer_rx;
        STORAGE_RX cache_rx;
        USER_MANAGER_RX user_manager_rx;

        KERNEL_TX kernel_tx;
        ROUTER_TX router_tx;
        CONFIG_TX config_tx;
        STORAGE_TX storage_tx;
        STORAGE_TX storage_writer_tx;
        STORAGE_TX cache_tx;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask() ;

        /// @name Helper member functions.
private:
        void handle_kernel_input();
        void handle_config_input();
        void handle_router_input();
        void wait_for_final_messages();
        void handle_kernel_send_journal_info();
        std::string send_journal_info_for_uuids(const std::string& uuids,
                        Poco::Timestamp last_seen);
        std::string send_journal_info_by_last_seen_time(
                        Poco::Timestamp last_seen);
        void initialize(const backup_cfg_package& in);
        void initialize_key(const std::string& key, const std::string& iv);
        void initialize_key(const Poco::JSON::Object::Ptr& obj);
        void initialize_device_json(const std::string& device_json_path);
        void parse_device_json();
        void save_device_json();
        void on_timer(Poco::Timer&);
        void on_clean_up_timer(Poco::Timer&);
        void clean_up_expired_data();
        void handle_clean_up_time_expiration();
        void handle_expired_data(const std::string uuid,
                        Poco::JSON::Object::Ptr obj,
                        std::vector<std::string>& d_uuids,
                        std::vector<std::string>& s_uuids);
        void configure_and_start_timer(uint16_t periodic_interval);
        void handle_timer_expiration();
        void handle_kernel_delete_from_device_json(const Poco::UUID& uuid,
                        uint32_t version, Poco::UInt64 last_updated);
        void handle_add_in_device_json(const kernel_backup_package& in);
        void handle_add_in_device_json(const backup_user_manager_package& in);
        void update_info_for_deleted_uuid(const std::string& uuid,
                        const std::string last_updated,
                        const std::string dversion);
        void handle_kernel_get_journal_info(const kernel_backup_package& in);
        bool should_get_update(const std::string& uuid,
                        const std::string& version,
                        const std::string& last_updated);
        void post_storage_delete_request_for_uuids(
                        const std::vector<std::string>& uuids);
        void post_kernel_send_uuids(Poco::JSON::Object::Ptr obj, ID32 rid);
        void handle_backup_send_uuids(const kernel_backup_package& in);
        void handle_storage_get_uuid(const kernel_storage_package& in);
        void handle_user_manager_input();
        void handle_storage_input();
        void post_storage_remove_from_cache_request(const std::string& uuid);
        void post_kernel_notify_delete(const std::string& uuid, Poco::UInt64 v);
        void handle_kernel_dump_user_info(const kernel_backup_package& in);
        std::string construct_journal_info_line(Poco::JSON::Object::Ptr obj,
                Poco::Timestamp last_seen, const std::string& uuid);


        /// @name Private data
private:

        /**
         * @brief Path of device json
         */
        std::string m_device_json_path;

        /**
         * @brief Key path of device json
         */
        std::string m_device_json_key_path;

        Poco::JSON::Object::Ptr m_device_json;
        Poco::JSON::Object::Ptr m_storage_data;

        bool m_device_json_updated;

        Poco::Timer m_timer;
        std::atomic<bool> m_timeout;

        Poco::Timer m_clean_up_timer;
        std::atomic<bool> m_clean_up_timeout;

        Poco::Crypto::Cipher::Ptr m_device_json_cipher;

        /// @name Special member functions
public:
        /// Default constructor
        backup();

        /// Destructor
        virtual ~backup();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        backup(const backup&);

        /// This class is not assignable
        backup& operator=(const backup&);

        /// This class is not copy-constructible
        backup(const backup&&);

        /// This class is not assignable
        backup& operator=(const backup&&);

}; // class leviathan::metax::backup

#endif // LEVIATHAN_METAX_BACKUP_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

