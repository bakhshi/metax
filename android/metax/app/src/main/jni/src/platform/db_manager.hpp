/**
 * @file src/platform/db_manager.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::platform::db_manager
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_DB_MANAGER_HPP
#define LEVIATHAN_PLATFORM_DB_MANAGER_HPP

// Headers from this project
#include "default_package.hpp"

// Headers from other projects
#include <Poco/Timer.h>

// Headers from third party libraries

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace platform {
                struct db_manager;
        }
}

/**
 * @brief Abstract class for storage implementation.
 */
struct leviathan::platform::db_manager
{
        /// @name Public interface
public:
        /**
         * @brief Get node by uuid.
         *
         * @param uuid - uuid of node which should be get
         */
        virtual default_package get(const std::string& uuid) = 0;

        /**
         * @brief Create new node.
         *
         * @param uuid - id of new node.
         * @param p - new node settings.
         */
        virtual void create(const std::string& uuid,
                        const default_package& p) = 0;

        /**
         * @brief Checks the node existence.
         *
         * @param uuid - uuid of node which existence should be checked.
         * @param should_look_up_cache - if true then should look up in cache
         * too, otherwise should look up only in storage.
         * @return - true if node with specified uuid exists, false otherwise.
         */
        virtual bool look_up(const std::string& uuid,
                        bool should_look_up_cache = true) = 0;

        /**
         * @brief Deletes the node with specified uuid.
         *
         * @param uuid - id of node which should be deleted.
         */
        virtual void delete_uuid(const std::string& uuid) = 0;

        /**
         * @brief Reduces the size with specified uuid.
         *
         * @param uuid - id of size which should be reduced.
         */
        virtual void reduce_uuid_size(const std::string& uuid) = 0;

        /**
         * @brief Copies the specified node into the specified new node.
         *
         * @param old_uuid - uuid of the node which should be copied.
         * @param new_uuid - uuid of the new in which the old uuid should be
         * copied.
         */
        virtual void copy(const std::string& old_uuid,
                        const std::string& new_uuid) = 0;

        /**
         * @brief Checks the node existence in cache.
         *
         * @param uuid - uuid of node which existence should be checked.
         * @return - true if node with specified uuid exists in cache,
         * false otherwise.
         */
        virtual bool look_up_in_cache(const std::string& uuid) = 0;

        /**
         * @brief Adds specified node in cache.
         *
         * @param uuid - id of node which should be added in cache.
         * @param p - node settings.
         */
        virtual void add_in_cache(const std::string& uuid,
                        const default_package& p) = 0;

        /**
         * @brief Deletes the node from cache with specified uuid.
         *
         * @param uuid - id of node which should be deleted from cache.
         */
        virtual void delete_uuid_from_cache(const std::string& uuid) = 0;

        /**
         * @brief - Cleans up cache data if actual size of cache size is
         * greater than given cache size limit.
         *
         */
        virtual void cleanup_cache() = 0;

        /// @return Actual used cache size in bytes.
        virtual Poco::UInt64 get_cache_size() const = 0;

        /// @return Actual used cache size in bytes.
        virtual Poco::UInt64 get_storage_size() const = 0;

        /**
         * @brief - Moves all uuids for the cache directory into db path
         *
         * @return - true if no error occurred, false if error occurred
         */
        virtual bool move_cache_to_storage() = 0;

        /**
         * @brief - Returns the list of all uuids existing in cache and storage
         * directories.
         *
         * @return - the list of uuids separated by comma.
         */
        virtual std::string get_all_uuids() const = 0;

        /// @name Special member functions
public:
        /// @brief Constructor
        db_manager();

        /// @brief Destructor
        virtual ~db_manager();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        db_manager(const db_manager&);

        /// This class is not assignable
        db_manager& operator=(const db_manager&);

}; // class leviathan::platform::db_manager

#endif // LEVIATHAN_PLATFORM_DB_MANAGER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
