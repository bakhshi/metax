/**
 * @file src/metax/user_manager.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::user_manager
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

#ifndef LEVIATHAN_METAX_USER_MANAGER_HPP
#define LEVIATHAN_METAX_USER_MANAGER_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/Crypto/Cipher.h>
#include <Poco/Timer.h>

// Headers from standard libraries
#include <map>
#include <atomic>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct user_manager;
        }
}

/**
 * @brief Implements user metadata management in Metax. It mainly includes AES
 * keys for the saved data or for data shared with this instance of Metax.
 */
struct leviathan::metax::user_manager:
        public leviathan::platform::csp::task
{
        /// @name Types used in the class.
public:
        /// Public type for messages
        using CONFIG_RX = platform::csp::input<backup_cfg_package>;
        using KERNEL_RX = platform::csp::input<kernel_user_manager_package>;

        using KERNEL_TX = platform::csp::output<kernel_user_manager_package>;

        using BACKUP_TX = platform::csp::output<backup_user_manager_package>;

        /// @name Public members
public:
        KERNEL_RX kernel_rx;
        CONFIG_RX config_rx;

        KERNEL_TX kernel_tx;
        BACKUP_TX backup_tx;

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

        void process_kernel_input();
        void handle_kernel_save();
        void handle_kernel_delete();
        void update_version();
        void create_cipher(Poco::JSON::Object::Ptr info);
        void create_cipher(const std::string& key, const std::string& iv);
        void generate_user_json();
        void save_in_file(const std::string c);
        std::string save_user_json(metax::command cmd = metax::update);
        void send_user_json_to_kernel(const std::string& uj);
        void request_user_json_from_network();
        void backup_user_json(const std::string& euj,
                        metax::command cmd, Poco::UInt64 lu);
        void update_device_json(metax::command cmd);
        void init_user_json(const backup_cfg_package& in);
        void start_timer();
        /**
         * @brief - Checks whether the user json has correct format.
         * Throws exception if it is not
         */
        void validate_user_json();
        void on_timer(Poco::Timer&);
        void handle_timer_expiration();
        void handle_kernel_dump_user_info(
                        const kernel_user_manager_package& in);
        void handle_kernel_update();
        void handle_user_json_final_save();
        std::string decrypt_user_json();
        void process_user_json_get();
        void parse_user_json_from_string(const std::string& uj);
        void parse_recieved_user_json(const kernel_user_manager_package& in);
        void process_kernel_connected_request();
        void handle_kernel_save_fail();
        void handle_kernel_save_confirm();

        /// @name Private data
private:
        uint16_t m_metadata_dump_timer;
        std::string m_user_json_path;
        std::string m_user_json_info_path;
        std::string m_storage_path;
        std::string m_user_json_uuid;
        Poco::JSON::Object::Ptr m_user_json;
        Poco::JSON::Object::Ptr m_owned_data;
        std::string m_enc_type;
        Poco::Crypto::Cipher::Ptr m_user_json_cipher;
        bool m_user_json_updated;
        bool m_should_get_user_json;
        bool m_should_save_user_json;
        bool m_is_user_json_saved;

        /*
         * Upon timeout the user_json is dumped into file
         */
        Poco::Timer m_timer;
        std::atomic<bool> m_timeout;

        /// @name Special member functions
public:
        /// Default constructor
        user_manager();

        /// Destructor
        virtual ~user_manager();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        user_manager(const user_manager&);

        /// This class is not assignable
        user_manager& operator=(const user_manager&);

        /// This class is not copy-constructible
        user_manager(const user_manager&&);

        /// This class is not assignable
        user_manager& operator=(const user_manager&&);

}; // class leviathan::metax::user_manager

#endif // LEVIATHAN_METAX_USER_MANAGER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

