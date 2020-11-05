
/**
 * @file src/metax/storage.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::storage
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "storage.hpp"

// Headers from other projects
#include <platform/utils.hpp>
#include <platform/db_json.hpp>

// Headers from third party libraries
#include <Poco/ThreadPool.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>

// Headers from standard libraries
#include <string>
#include <sstream>


std::unique_ptr<leviathan::platform::db_manager>
leviathan::metax::storage::s_db = nullptr;


char* leviathan::metax::storage::
get_metadata(const UUID& ) const
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return nullptr;
}

void leviathan::metax::storage::
send_ack_with_metadata(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = ack;
        //out = get_metadata(in.uuid);
        std::string mdt = "This is a metadata for the requested node ";
        out.set_payload(mdt);
        out.uuid = in.uuid;
        kernel_tx.commit();
        METAX_INFO("Found in storage - uuid: " + in.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
send_nack(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = nack;
        out.uuid = in.uuid;
        kernel_tx.commit();
        METAX_INFO("NOT found in storage - uuid: " + in.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
look_up_data_in_db(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        if (s_db->look_up(in.uuid.toString(), in.check_cache)) {
                send_ack_with_metadata(in);
        } else {
                send_nack(in);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::storage::
look_up_data_in_db(const Poco::UUID& u, bool check_cache)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        return s_db->look_up(u.toString(), check_cache);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
get_data_from_db(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_data;
        out.uuid = in.uuid;
        poco_assert(nullptr != s_db);
        try {
                out = s_db->get(in.uuid.toString());
        } catch (Poco::Exception& e) {
                METAX_ERROR("File not found storage: " + in.uuid.toString());
                out.cmd = metax::failed;
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
get_data_from_db_for_router_async(ID32 rid, Poco::UUID u, bool check_cache)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        metax::command cmd = metax::get_data;
        platform::default_package d;
        if (s_db->look_up(u.toString(), check_cache)) {
                METAX_INFO("Found in storage - uuid: " + u.toString());
                try {
                        d = s_db->get(u.toString());
                } catch (...) {
                        cmd = metax::refuse;
                        METAX_ERROR("Error while reading file from storage: "
                                                                + u.toString());
                }
        } else {
                cmd = metax::refuse;
                METAX_INFO("File not found: " + u.toString());
        }
        Poco::ScopedLock<Poco::Mutex> m(m_router_mutex);
        kernel_storage_package& out = *router_tx;
        out.cmd = cmd;
        out.request_id = rid;
        out.uuid = u;
        out = d;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
send_uuids_for_sync_process(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::start_storage_sync;
        poco_assert(nullptr != s_db);
        out.set_payload(s_db->get_all_uuids());
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
send_uuids(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != in.message.get());
        poco_assert(nullptr != s_db);
        Poco::StringTokenizer st(in.message.get(), ",");
        for (auto i : st) {
                try {
                        if (s_db->look_up(i)) {
                                kernel_storage_package& out = *backup_tx;
                                out = s_db->get(i);
                                out.request_id = in.request_id;
                                out.cmd = metax::get_uuid;
                                out.uuid.parse(i);
                                backup_tx.commit();
                        }
                } catch (Poco::Exception& e) {
                        METAX_ERROR("File not found in storage: " + i);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
save_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_storage_package& in = *kernel_rx;
        poco_assert(metax::save_data == in.cmd);
        poco_assert(nullptr != s_db);
        command cmd = metax::save_confirm;
        std::string msg;
        try {
                s_db->reduce_uuid_size(in.uuid.toString());
                s_db->create(in.uuid.toString(), in);
                post_cfg_manager_write_storage_cfg_request();
        } catch (const Poco::Exception& e) {
                cmd = 28 == e.code() ? metax::no_space : metax::failed;
                msg = e.message();
                METAX_ERROR(msg);
        }
        if (in.should_send_response) {
                kernel_storage_package& out = *kernel_tx;
                out.uuid = in.uuid;
                out.request_id = in.request_id;
                out.cmd = cmd;
                out.set_payload(msg);
                kernel_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
save_data_async_for_router(ID32 id, Poco::UUID uuid,
                                                platform::default_package data)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        try {
                s_db->create(uuid.toString(), data);
                Poco::ScopedLock<Poco::Mutex> m(m_router_mutex);
                kernel_storage_package& out = *router_tx;
                out.uuid = uuid;
                out.request_id = id;
                out.cmd = metax::save_confirm;
                router_tx.commit();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Could not save " + uuid.toString()
                                + " uuid:" + e.message());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
update_data_async_for_router(ID32 id, Poco::UUID uuid,
                                                platform::default_package data)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        try {
                s_db->reduce_uuid_size(uuid.toString());
                s_db->create(uuid.toString(), data);
                Poco::ScopedLock<Poco::Mutex> m(m_router_mutex);
                kernel_storage_package& out = *router_tx;
                out.uuid = uuid;
                out.request_id = id;
                out.cmd = metax::updated;
                router_tx.commit();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Could not update " + uuid.toString()
                                + " uuid:" + e.message());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
add_in_cache()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_storage_package& in = *kernel_rx;
        poco_assert(metax::add_in_cache == in.cmd);
        poco_assert(nullptr != s_db);
        s_db->add_in_cache(in.uuid.toString(), in);
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
post_cfg_manager_write_storage_cfg_request()
{
        METAX_TRACE(__FUNCTION__);
        Poco::ScopedLock<Poco::Mutex> m(m_config_mutex);
        (*config_tx).cmd = metax::write_storage_cfg;
        (*config_tx).cache_size = s_db->get_cache_size();
        (*config_tx).storage_size = s_db->get_storage_size();
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
copy_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_storage_package& in = *kernel_rx;
        poco_assert(metax::copy == in.cmd);
        poco_assert(nullptr != in.message.get());
        poco_assert(nullptr != s_db);
        const ID32& id = in.request_id;
        kernel_storage_package& out = *kernel_tx;
        try {
                if (s_db->look_up(in.uuid.toString(), false)) {
                        std::string s(in.message.get(), in.size);
                        s_db->copy(in.uuid.toString(), s);
                        out.cmd = metax::copy_succeeded;
                } else {
                        out.cmd = metax::nack;
                }
        } catch (const Poco::Exception& e) {
                out.cmd = 28 == e.code() ? metax::no_space : metax::failed;
                out.set_payload(e.message());
                METAX_ERROR(e.message());
        }
        out.uuid = in.uuid;
        out.request_id = id;
        kernel_tx.commit();
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
delete_uuids()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(backup_rx.has_data());
        const kernel_storage_package& in = *backup_rx;
        poco_assert(metax::del_uuids == in.cmd);
        poco_assert(nullptr != in.message.get());
        poco_assert(nullptr != s_db);
        Poco::StringTokenizer st(in.message.get(), ",");
        for (auto i : st) {
                if (s_db->look_up(i)) {
                        s_db->delete_uuid(i);
                }
        }
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
delete_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_storage_package& in = *kernel_rx;
        poco_assert(metax::del == in.cmd);
        poco_assert(nullptr != s_db);
        kernel_storage_package& out = *kernel_tx;
        out.uuid = in.uuid;
        out.cmd = nack;
        out.request_id = in.request_id;
        std::string u = in.uuid.toString();
        try {
                if (s_db->look_up_in_cache(u)) {
                        s_db->delete_uuid_from_cache(u);
                        post_cfg_manager_write_storage_cfg_request();
                }
                if (s_db->look_up(u, false)) {
                        s_db->delete_uuid(u);
                        out.cmd = metax::deleted;
                        post_cfg_manager_write_storage_cfg_request();
                }
        } catch (...) {
                METAX_ERROR("Unable to delete uuid: " + u);
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
delete_data_async_for_router(ID32 id, Poco::UUID uuid, bool respond)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        s_db->delete_uuid(uuid.toString());
        if (respond) {
                Poco::ScopedLock<Poco::Mutex> m(m_router_mutex);
                kernel_storage_package& out = *router_tx;
                out.request_id = id;
                out.cmd = metax::deleted;
                out.uuid = uuid;
                router_tx.commit();
        }
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
delete_data_from_cache(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::delete_from_cache == in.cmd);
        poco_assert(nullptr != s_db);
        s_db->delete_uuid_from_cache(in.uuid.toString());
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
move_cache_to_storage()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_storage_package& in = *kernel_rx;
        bool st = s_db->move_cache_to_storage();
        post_cfg_manager_write_storage_cfg_request();
        kernel_storage_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = move_cache_to_storage_completed;
        out.status = st;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
handle_config_storage()
{
        METAX_TRACE(__FUNCTION__);
        const storage_cfg_package& in = *config_rx;
        poco_assert(metax::storage_cfg == in.cmd);
        std::string storage_path = in.message.get();
        // TODO - get storage type from configuration manager.
        uint64_t l = (uint64_t)(1 << 30) * in.cache_size_limit_in_gb;
        if (in.size_limit_in_gb < 0) {
                s_db.reset(new platform::db_json(storage_path,
                                in.cache_size, in.storage_size,
                                in.cache_age, l, in.size_limit_in_gb));
        } else {
                int64_t storage_l = (int64_t)(1 << 30) * in.size_limit_in_gb;
                s_db.reset(new platform::db_json(storage_path,
                                in.cache_size, in.storage_size,
                                in.cache_age, l, storage_l));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
handle_kernel_input()
{
        if (! kernel_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        if (nullptr == s_db) {
                Poco::Thread::yield();
                return;
        }
        try {
                process_kernel_input();
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
        kernel_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
handle_backup_input()
{
        if (! backup_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        if (nullptr == s_db) {
                Poco::Thread::yield();
                return;
        }
        try {
                process_backup_input();
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
        backup_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
process_kernel_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_storage_package& in = *kernel_rx;
        switch (in.cmd) {
                case metax::look_up : {
                        look_up_data_in_db(in);
                        break;
                } case metax::get : {
                        get_data_from_db(in);
                        break;
                } case metax::save_data: {
                        save_data();
                        break;
                } case metax::del: {
                        delete_data();
                        break;
                } case metax::copy: {
                        copy_data();
                        break;
                } case metax::add_in_cache: {
                        add_in_cache();
                        break;
                } case metax::delete_from_cache: {
                        delete_data_from_cache(in);
                        break;
                } case metax::move_cache_to_storage: {
                        move_cache_to_storage();
                        break;
                } case metax::send_uuids: {
                        send_uuids(in);
                        break;
                } case metax::start_storage_sync: {
                        send_uuids_for_sync_process(in);
                        break;
                } default : {
                        METAX_WARNING("Unhandled command from kernel");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
process_backup_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(backup_rx.has_data());
        const kernel_storage_package& in = *backup_rx;
        switch (in.cmd) {
                case metax::del_uuids: {
                        delete_uuids();
                        break;
                } case metax::delete_from_cache: {
                        delete_data_from_cache(in);
                        break;
                } default : {
                        METAX_WARNING("Unhandled command from backup");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
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

void leviathan::metax::storage::
process_config_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        switch ((*config_rx).cmd) {
                case metax::storage_cfg: {
                        handle_config_storage();
                        break;
                } default: {
                        METAX_WARNING(
                         "Command from configuration manager not handled yet.");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
cleanup_cache(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != s_db);
        s_db->cleanup_cache();
        post_cfg_manager_write_storage_cfg_request();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::storage::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        if (is_cache_owner) {
                Poco::TimerCallback<storage> onTimerCallback(*this,
                                &storage::cleanup_cache);
                m_cache_timer.start(onTimerCallback,
                                Poco::ThreadPool::defaultPool());
        }
        while (true) {
                // wait to read data
                if (!wait()) {
                        break;
                }
                handle_config_input();
                handle_kernel_input();
                handle_backup_input();
        }
        if (is_cache_owner) {
                m_cache_timer.stop();
        }
        finish();
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
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

leviathan::metax::storage::
storage(const std::string& n, bool o)
        : platform::csp::task(n, Poco::Logger::get("metax." + n))
        , kernel_rx(this)
        , config_rx(this)
        , backup_rx(this)
        , kernel_tx(this)
        , config_tx(this)
        , router_tx(this)
        , backup_tx(this)
        , m_cache_timer(60000, 60000) // 1m
        , is_cache_owner(o)
        , m_router_mutex()
        , m_config_mutex()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::storage::
~storage()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

