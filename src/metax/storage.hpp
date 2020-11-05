/**
 * @file src/metax/storage.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::storage
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_STORAGE_HPP
#define LEVIATHAN_METAX_STORAGE_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/Timer.h>

// Headers from standard libraries
#include <memory>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct storage;
        }
        namespace platform {
                struct db_manager;
        }
}

/**
 * @brief Performs all disc operations for saving and getting files to/from hard
 * drives.
 *
 */
struct leviathan::metax::storage:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        using CONFIG_RX = platform::csp::input<storage_cfg_package>;
        using CONFIG_TX = platform::csp::output<storage_cfg_package>;

        using KERNEL_RX = platform::csp::input<kernel_storage_package>;
        using KERNEL_TX = platform::csp::output<kernel_storage_package>;

        using ROUTER_TX = platform::csp::output<kernel_storage_package>;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

        /**
         * @brief Reads uuid from storage and sends response to router. This
         * functions is run from a separate thread, so before pushing data into
         * router, the router tx is locked by mutex
         *
         * @param id - request id
         * @param uuid - uuid to get from storage
         * @param check_cache - whether to check uuid existence also in cache
         */
        void get_data_from_db_for_router_async(ID32 id,
                                        Poco::UUID uuid, bool check_cache);

        /**
         * @brief Saves uuid into storage and sends response to router. This
         * functions is run from a separate thread, so before pushing data into
         * router, the router tx is locked by mutex
         *
         * @param id - request id
         * @param uuid - uuid to be saved
         * @param data - payload to be saved
         */
        void save_data_async_for_router(ID32 id, Poco::UUID uuid,
                                                platform::default_package data);

        /**
         * @brief Updates uuid into storage and sends response to router. This
         * functions is run from a separate thread, so before pushing data into
         * router, the router tx is locked by mutex
         *
         * @param id - request id
         * @param uuid - uuid to be updated
         * @param data - payload to be saved
         */
        void update_data_async_for_router(ID32 id, Poco::UUID uuid,
                                                platform::default_package data);

        /**
         * @brief Deletes uuid from storage and sends response to router. This
         * functions is run from a separate thread, so before pushing data into
         * router, the router tx is locked by mutex
         *
         * @param id - request id
         * @param uuid - uuid to be deleted
         * @param respond - if false, confirmation response isn't sent to router
         */
        void delete_data_async_for_router(ID32 id,
                                        Poco::UUID uuid, bool respond);

        /**
         * @brief Searches uuid in storage.
         *
         * @param uuid - uuid to be searched
         * @param check_cache - whether to look also in cache
         */
        bool look_up_data_in_db(const Poco::UUID& uuid, bool check_cache);

        /// @name Helper member functions.
private:
        void handle_kernel_input();
        void process_kernel_input();
        void handle_config_input();
        void process_config_input();

        char* get_metadata(const UUID& uuid) const;
        void send_ack_with_metadata(const kernel_storage_package& in);
        void send_nack(const kernel_storage_package& in);
        void look_up_data_in_db(const kernel_storage_package& in);
        void get_data_from_db(const kernel_storage_package& in);
        void save_data();
        void handle_config_storage();
        void delete_data();
        void delete_uuids();
        void send_uuids(const kernel_storage_package& in);
        void copy_data();
        void add_in_cache();
        void delete_data_from_cache(const kernel_storage_package& in);
        void move_cache_to_storage();
        void post_cfg_manager_write_storage_cfg_request();
        void cleanup_cache(Poco::Timer&);
        void send_uuids_for_sync_process(const kernel_storage_package& in);
        void handle_backup_input();
        void process_backup_input();

        /// @name Public members
public:
        KERNEL_RX kernel_rx;
        CONFIG_RX config_rx;
        KERNEL_RX backup_rx;

        KERNEL_TX kernel_tx;
        CONFIG_TX config_tx;
        ROUTER_TX router_tx;
        KERNEL_TX backup_tx;

        /// @name Private data
private:
        Poco::Timer m_cache_timer;
        bool is_cache_owner;

        static std::unique_ptr<platform::db_manager> s_db;

        Poco::Mutex m_router_mutex;
        Poco::Mutex m_config_mutex;

        /// @name Special member functions
public:
        /// Default constructor
        storage(const std::string& n, bool o = false);

        /// Destructor
        virtual ~storage();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        storage(const storage&);

        /// This class is not assignable
        storage& operator=(const storage&);

        /// This class is not copy-constructible
        storage(const storage&&);

        /// This class is not assignable
        storage& operator=(const storage&&);

}; // class leviathan::metax::storage

#endif // LEVIATHAN_METAX_STORAGE_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

