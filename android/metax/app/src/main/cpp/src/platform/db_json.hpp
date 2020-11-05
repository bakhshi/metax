/**
 * @file src/platform/db_json.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::platform::db_json
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_DB_JSON_HPP
#define LEVIATHAN_PLATFORM_DB_JSON_HPP

// Headers from this project
#include "db_manager.hpp"
#include "default_package.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Path.h>

// Headers from standard libraries
#include <atomic>

// Forward declarations
namespace leviathan {
        namespace platform {
                class db_json;
        }
}

/**
 * @brief JSON database implementation.
 */
class leviathan::platform::db_json
        : public leviathan::platform::db_manager
{
        /// @name Public interface
public:

        /**
         * @brief Get node by uuid.
         *
         * @param uuid - uuid of node which should be get
         */
        leviathan::platform::default_package get(const std::string& uuid);

        /**
         * @brief Create new node.
         *
         * @param uuid - id of new node.
         * @param p - new node settings.
         */
        void create(const std::string& uuid, const default_package& p);

        /**
         * @brief Checks the node existence.
         *
         * @param uuid - uuid of node which existence should be checked.
         * @param should_look_up_cache - if true then should look up in cache
         * too, otherwise should look up only in storage.
         * @return - true if node with specified uuid exists, false otherwise.
         */
        bool look_up(const std::string& uuid, bool should_look_up_cache = true);

        /**
         * @brief Deletes the node with specified uuid.
         *
         * @param uuid - id of node which should be deleted.
         */
        void delete_uuid(const std::string& uuid);

        /**
         * @brief Reduces the size with specified uuid.
         *
         * @param uuid - id of size which should be reduced.
         */
        void reduce_uuid_size(const std::string& uuid);

        /**
         * @brief Copies the specified node into the specified new node.
         *
         * @param old_uuid - uuid of the node which should be copied.
         * @param new_uuid - uuid of the new in which the old uuid should be
         * copied.
         */
        void copy(const std::string& old_uuid, const std::string& new_uuid);

        /**
         * @brief Checks the node existence in cache.
         *
         * @param uuid - uuid of node which existence should be checked.
         * @return - true if node with specified uuid exists in cache,
         * false otherwise.
         */
        bool look_up_in_cache(const std::string& uuid);

        /**
         * @brief Deletes the node from cache with specified uuid.
         *
         * @param uuid - id of node which should be deleted.
         */
        void delete_uuid_from_cache(const std::string& uuid);

        /**
         * @brief Adds specified node in cache.
         *
         * @param uuid - id of node which should be added.
         * @param p - node settings.
         */
        void add_in_cache(const std::string& uuid, const default_package& p);

        /**
         * @brief - Cleans up cache data if actual size of cache size is
         * greater than given cache size limit.
         *
         * Details:
         * Cache data are deleted by last modified data. The earliest created
         * files are deleted until cache size is not greater than given cache
         * size limit.
         *
         */
        void cleanup_cache();

        /**
         * @brief - Moves all uuids for the cache directory into db path
         *
         * @return - true if no error occurred, false if error occurred
         */
        bool move_cache_to_storage();

        /**
         * @brief - Returns the list of all uuids existing in cache and storage
         * directories.
         *
         * @return - the list of uuids separated by comma
         */
        std::string get_all_uuids() const;

        /**
         * @return Actual used cache size in bytes
         */
        Poco::UInt64 get_cache_size() const;

        /**
         * @return Actual used storage size in bytes
         */
        Poco::UInt64 get_storage_size() const;

        /// @name Private member functions
private:
        void create_storage_dir();
        void create_cache_dir();

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         *
         * @param path - path to json database.
         * @param cache_size - Max size of cache in GB, -1 means unlimited
         * @param storage_size - Current size of the storage in GB
         * @param l - cache age in minutes, data added in cache earlier than
         * will be removed, 0 means ignore cache age
         * @param t - cache limit in GB
         * @param m - storage limit in GB
         *
         */
        db_json(const std::string& path, uint64_t cache_size, uint64_t storage_size,
                        uint64_t l, uint64_t t = (1 << 30), int64_t m = (1 << 30));


        /// Destructor
        virtual ~db_json();

        /// @name Private Membres
private:
        Poco::Path m_db_path;
        Poco::Path m_cache_path;
#if defined(__SH4__) || defined(__arm__)
        std::atomic<unsigned long> m_cache_size;
        std::atomic<unsigned long> m_storage_size;
        std::atomic<unsigned long> m_cache_age;
#else
        std::atomic<uint64_t> m_cache_size;
        std::atomic<uint64_t> m_storage_size;
        std::atomic<uint64_t> m_cache_age;
#endif
        uint64_t m_cache_limit;
        int64_t m_storage_limit;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        db_json(const db_json&);

        /// This class is not assignable
        db_json& operator=(const db_json&);

}; // class leviathan::platform::db_json

#endif // LEVIATHAN_PLATFORM_DB_JSON_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
