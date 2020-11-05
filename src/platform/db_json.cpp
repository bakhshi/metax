/**
 * @file src/platform/db_json.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::platform::db_json
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "db_json.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/File.h>
#include <Poco/ThreadPool.h>
#include <Poco/UUID.h>
#include <Poco/FileStream.h>

// Headers from standard libraries
#include <fstream>
#include <iostream>

leviathan::platform::default_package
leviathan::platform::db_json::
get(const std::string& uuid)
{
        if (! look_up(uuid)) {
                throw Poco::Exception("Not found!");
        }
        Poco::Path p = look_up_in_cache(uuid) ? m_cache_path : m_db_path;
        p.append(uuid);
        std::ifstream ifs(p.toString(), std::ios::binary);
        poco_assert(ifs.is_open());
        ifs.seekg(0, std::ios::end);
        int32_t length = (uint32_t)ifs.tellg();
        default_package dp;
        if (0 == length) {
                return dp;
        }
        ifs.seekg(0, std::ios::beg);
        char* buffer = new char [length + 1];
        ifs.read(buffer, length);
        buffer[length] = '\0';
        ifs.close();
        dp.resize(length);
        dp.set_payload(buffer);
        return dp;
}

void leviathan::platform::db_json::
create(const std::string& uuid, const default_package& content)
{
        Poco::Path p = m_db_path;
        p.append(uuid);
        Poco::File f(p);
        if (m_storage_limit >= 0) {
                if (m_storage_size + content.size > m_storage_limit) {
                        throw Poco::Exception("Failed to save: No space in storage", 28);
                }
        }
        std::ofstream ofs(p.toString(), std::ios::binary);
        ofs.write(content.message, (int32_t)content.size);
        if (ofs.fail()) {
                throw Poco::Exception("Could not save in " + p.toString());
        }
        ofs.close();
        m_storage_size += content.size;
}

void leviathan::platform::db_json::
add_in_cache(const std::string& uuid, const default_package& content)
{
        Poco::Path p(m_cache_path);
        p.append(uuid);
        std::ofstream ofs(p.toString(), std::ios::binary);
        ofs.write(content.message, (int32_t)content.size);
        ofs.close();
        m_cache_size += content.size;
}

void leviathan::platform::db_json::
delete_uuid_from_cache(const std::string& uuid)
{
        Poco::Path p(m_cache_path);
        p.append(uuid);
        Poco::File f(p);
        if (f.exists()) {
                m_cache_size -= (unsigned long)f.getSize();
                f.remove();
        }
}

void leviathan::platform::db_json::
delete_uuid(const std::string& uuid)
{
        if (look_up_in_cache(uuid)) {
                delete_uuid_from_cache(uuid);
        }
        Poco::Path p = m_db_path;
        p.append(uuid);
        Poco::File f(p);
        if (f.exists()) {
                m_storage_size -= (unsigned long)f.getSize();
                f.remove();
        }
}

void leviathan::platform::db_json::
reduce_uuid_size(const std::string& uuid)
{
        Poco::Path p = m_db_path;
        p.append(uuid);
        Poco::File f(p);
        if (f.exists()) {
                m_storage_size -= (unsigned long)f.getSize();
        }
}

void leviathan::platform::db_json::
copy(const std::string& old_uuid, const std::string& new_uuid)
{
        Poco::Path p = m_db_path;
        p.append(old_uuid);
        Poco::File f(p);
        poco_assert(f.exists());
        Poco::File::FileSize s = f.getSize();
        if (m_storage_size + s > m_storage_limit) {
                throw Poco::Exception("Failed to copy: No space in storage", 28);
        }
        f.copyTo(m_db_path.toString() + "/" + new_uuid);
        m_storage_size += s;
}

bool leviathan::platform::db_json::
look_up_in_cache(const std::string& uuid)
{
        Poco::Path p(m_cache_path);
        p.append(uuid);
        Poco::File f(p);
        return f.exists();
}

bool leviathan::platform::db_json::
look_up(const std::string& uuid, bool should_look_up_cache)
{
        // At first check in cache.
        if (should_look_up_cache && look_up_in_cache(uuid)) {
                return true;
        }
        // Check in storage if missing in cache
        Poco::Path p1 = m_db_path;
        p1.append(uuid);
        Poco::File f1(p1);
        return f1.exists();
}

void leviathan::platform::db_json::
create_storage_dir()
{
        Poco::File f(m_db_path);
        try {
                f.createDirectories();
                poco_assert(f.exists());
        } catch (Poco::Exception& e) {
                std::cerr << e.displayText() << std::endl;
        }
}

void leviathan::platform::db_json::
create_cache_dir()
try {
        Poco::File f(m_cache_path);
        f.createDirectories();
} catch (Poco::Exception& e) {
        std::cerr << e.displayText() << std::endl;
}

std::string leviathan::platform::db_json::
get_all_uuids() const
{
        std::vector<std::string> v1;
        std::vector<std::string> v2;
        Poco::File c(m_cache_path);
        Poco::File s(m_db_path);
        c.list(v1);
        s.list(v2);
        v1.insert(v1.end(), v2.begin(), v2.end());
        Poco::UUID u;
        std::string uuids;
        for (const std::string& i : v1) {
                if (u.tryParse(i)) {
                        if (uuids.empty()) {
                                uuids += i;
                        } else {
                                uuids += "," + i;
                        }
                }
        }
        return uuids;
}

void leviathan::platform::db_json::
cleanup_cache()
{
        std::vector<Poco::File> v;
        Poco::File c(m_cache_path);
        c.list(v);
        std::sort(v.begin(), v.end(),
                        [] (Poco::File const& f1, Poco::File const& f2)
                        { return f1.getLastModified() < f2.getLastModified();});
        Poco::Timestamp old;
        old -= m_cache_age;
        std::vector<Poco::File>::iterator b = v.begin();
        while (b != v.end() && ((0 != m_cache_age && b->getLastModified() < old)
                                || m_cache_size > m_cache_limit)) {
                m_cache_size -= (unsigned long)b->getSize();
                b->remove();
                ++b;
        }
}

bool leviathan::platform::db_json::
move_cache_to_storage()
{
        bool status = true;
        try {
                std::vector<Poco::File> v;
                Poco::File c(m_cache_path);
                c.list(v);
                std::vector<Poco::File>::iterator b = v.begin();
                for (; b != v.end(); ++b) {
                        try {
                                b->moveTo(m_db_path.toString());
                        } catch (Poco::Exception& e) {
                                std::cerr << e.displayText() << std::endl;
                                status = false;
                        }
                }
        } catch (Poco::FileNotFoundException& e) {
                std::cerr << e.displayText() << std::endl;
                status = false;
        }
        m_cache_size = 0;
        return status;
}

Poco::UInt64 leviathan::platform::db_json::
get_cache_size() const
{
        return m_cache_size;
}

Poco::UInt64 leviathan::platform::db_json::
get_storage_size() const
{
        return m_storage_size;
}

leviathan::platform::db_json::
db_json(const std::string& dir, uint64_t cache_size, uint64_t storage_size,
                                        uint64_t l, uint64_t t, int64_t m)
        : m_db_path("./storage")
        , m_cache_size(cache_size)
        , m_storage_size(storage_size)
        , m_cache_age(l * 60000000)
        , m_cache_limit(t)
        , m_storage_limit(m)
{
        if (! dir.empty()) {
                m_db_path.tryParse(dir);
        }
        m_cache_path = m_db_path;
        m_cache_path.append("cache");
        create_storage_dir();
        create_cache_dir();
}

leviathan::platform::db_json::
~db_json()
{
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

