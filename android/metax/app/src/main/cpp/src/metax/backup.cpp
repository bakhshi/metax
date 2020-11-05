/**
 * @file src/metax/backup.cpp
 *
 * @brief Implementation of the class @ref
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

// Headers from this project
#include "backup.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Thread.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CryptoStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/FileStream.h>
#include <Poco/ThreadPool.h>

// Headers from standard libraries

void leviathan::metax::backup::
handle_kernel_input()
{
        if (! kernel_rx.has_data()) {
                return;
        }
        if (nullptr == m_device_json) {
                Poco::Thread::yield();
                return;
        }
        METAX_TRACE(__FUNCTION__);
        const kernel_backup_package& in = *kernel_rx;
        switch (in.cmd) {
                case metax::send_journal_info: {
                        handle_kernel_send_journal_info();
                        break;
                } case metax::del: {
                        handle_kernel_delete_from_device_json(in.uuid,
                                        in.data_version, in.last_updated);
                        break;
                } case metax::save: {
                        handle_add_in_device_json(in);
                        break;
                } case metax::get_journal_info: {
                        handle_kernel_get_journal_info(in);
                        break;
                } case metax::dump_user_info: {
                        handle_kernel_dump_user_info(in);
                        break;
                } default: {
                        METAX_WARNING("Not handled command !!");
                }
        }
        kernel_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_user_manager_input()
{
        if (! user_manager_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        const backup_user_manager_package& in = *user_manager_rx;
        switch (in.cmd) {
                case metax::save:
                case metax::update: {
                        handle_add_in_device_json(in);
                        break;
                } case metax::finalize: {
                        return;
                } default: {
                        METAX_WARNING("Not handled command !!");
                }
        }
        user_manager_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_storage_input()
{
        if (! storage_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        const kernel_storage_package& in = *storage_rx;
        switch (in.cmd) {
                case metax::get_uuid: {
                        handle_storage_get_uuid(in);
                        break;
                } default: {
                        METAX_WARNING("Not handled command !!");
                }
        }
        storage_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_config_input()
{
        if (! config_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        const backup_cfg_package& in = *config_rx;
        switch (in.cmd) {
                case metax::backup_cfg: {
                        initialize(in);
                        break;
                } default: {
                        METAX_WARNING("This case should not be happened");
                }
        }
        config_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
initialize_key(const std::string& key, const std::string& iv)
try {
        METAX_TRACE(__FUNCTION__);
        namespace C = Poco::Crypto;
        std::string k = platform::utils::base64_decode(key);
        std::string i = platform::utils::base64_decode(iv);
        C::CipherKey::ByteVec v_iv(i.begin(), i.end());
        C::CipherKey::ByteVec key_vec(k.begin(), k.end());
        C::CipherKey aes_key("aes256", key_vec, v_iv);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        m_device_json_cipher = factory.createCipher(aes_key);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        std::terminate();
} catch (...) {
        METAX_FATAL("Unable to create device json cipher");
        std::terminate();
}

void leviathan::metax::backup::
initialize_key(const Poco::JSON::Object::Ptr& info)
try {
        METAX_TRACE(__FUNCTION__);
        namespace C = Poco::Crypto;
        std::string i = info->getValue<std::string>("aes_iv");
        std::string k = info->getValue<std::string>("aes_key");
        std::string key = platform::utils::base64_decode(k);
        std::string iv = platform::utils::base64_decode(i);
        C::CipherKey::ByteVec v_iv(iv.begin(), iv.end());
        C::CipherKey::ByteVec key_vec(key.begin(), key.end());
        C::CipherKey aes_key("aes256", key_vec, v_iv);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        m_device_json_cipher = factory.createCipher(aes_key);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        std::terminate();
} catch (...) {
        METAX_FATAL("Unable to create device json cipher");
        std::terminate();
}

void leviathan::metax::backup::
handle_storage_get_uuid(const kernel_storage_package& in)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get_uuid == in.cmd);
        poco_assert(nullptr != m_device_json);
        poco_assert(! m_storage_data.isNull());
        if (m_storage_data->has(in.uuid.toString())) {
                Poco::JSON::Object::Ptr u =
                        m_storage_data->getObject(in.uuid.toString());
                poco_assert(nullptr != u);
                kernel_backup_package& out = *kernel_tx;
                out.request_id = in.request_id;
                out.uuid = in.uuid;
                out.cmd = in.cmd;
                out = in;
                out.data_version = u->getValue<uint32_t>("version");
                out.last_updated = u->getValue<Poco::UInt64>("last_updated");
                out.expire_date = u->has("expiration_date") ?
                        u->getValue<Poco::UInt64>("expiration_date") : 0;
                kernel_tx.commit();
        } else {
                METAX_WARNING("Requested " + in.uuid.toString()
                                + " uuid missing in device json.");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (...) {
        METAX_WARNING("Unable to send requested uuid: " + in.uuid.toString());
}

void leviathan::metax::backup::
initialize(const backup_cfg_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(! in.metadata_path.empty());
        // Initialize key
        initialize_key(in.user_json_info_ptr);
        // Initialize device json pointer
        initialize_device_json(in.metadata_path);
        // Start timer to save device json in file periodically.
        configure_and_start_timer(in.periodic_interval);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
parse_device_json()
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_device_json_cipher);
        std::stringstream decrypt_sink;
        Poco::FileInputStream decrypt_source(m_device_json_path);
        Poco::Crypto::CryptoInputStream decryptor(decrypt_source,
                        m_device_json_cipher->createDecryptor());
        Poco::StreamCopier::copyStream(decryptor, decrypt_sink);
        m_device_json = platform::utils::
                parse_json<Poco::JSON::Object::Ptr>(decrypt_sink);
        decrypt_source.close();
        if (m_device_json->has("version")) {
                m_device_json->set("protocol_version",
                              m_device_json->getValue<Poco::UInt32>("version"));
                m_device_json->remove("version");
        }
        if (nullptr == m_device_json || ! m_device_json->has("storage")
                        || ! m_device_json->has("protocol_version")) {
                throw Poco::Exception("Invalid json file");
        }
        m_storage_data = m_device_json->getObject("storage");
        poco_assert(nullptr != m_storage_data);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::Exception& e) {
        METAX_FATAL("Unable to parse device json: " + e.message());
        std::terminate();
} catch (...) {
        METAX_FATAL("Unable to parse device json");
        std::terminate();
}

void leviathan::metax::backup::
on_timer(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        m_timeout = true;
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
on_clean_up_timer(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        m_clean_up_timeout = true;
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
configure_and_start_timer(uint16_t periodic_interval)
{
        METAX_TRACE(__FUNCTION__);
        Poco::TimerCallback<backup> expire_callback(*this,
                        &backup::on_clean_up_timer);
        m_clean_up_timer.start(expire_callback,
                        Poco::ThreadPool::defaultPool());
        if (0 < periodic_interval) {
                Poco::TimerCallback<backup> on_timer_callback(*this,
                                &backup::on_timer);
                m_timer.setPeriodicInterval(1000 * periodic_interval);
                m_timer.setStartInterval(1000 * periodic_interval);
                m_timer.start(on_timer_callback,
                                        Poco::ThreadPool::defaultPool());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
initialize_device_json(const std::string& device_json_path)
{
        METAX_TRACE(__FUNCTION__);
        m_device_json_path = device_json_path;
        poco_assert(! m_device_json_path.empty());
        Poco::File f2(m_device_json_path);
        if (f2.exists()) {
                parse_device_json();
        } else {
                m_device_json = new Poco::JSON::Object;
                m_device_json->set("protocol_version", 1);
                m_storage_data = new Poco::JSON::Object;
                m_device_json->set("storage", m_storage_data);
                m_device_json_updated = true;
                save_device_json();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
save_device_json()
{
        if (m_device_json_updated) {
                METAX_TRACE(__FUNCTION__);
                poco_assert(nullptr != m_device_json);
                poco_assert(nullptr != m_device_json_cipher);
                std::string t = m_device_json_path + ".tmp";
                {
                        Poco::FileOutputStream sink(t, std::ios::out |
                                                        std::ios::binary |
                                                        std::ios::trunc);
                        Poco::Crypto::CryptoOutputStream encryptor(sink,
                                       m_device_json_cipher->createEncryptor());
                        std::stringstream ss;
                        m_device_json->stringify(ss);
                        Poco::StreamCopier::copyStream(ss, encryptor);

                        //std::stringstream ss;
                        //m_device_json->stringify(ss);
                        //std::string e =
                        //        m_device_json_cipher->encryptString(ss.str());
                        //std::ofstream ofs(t, std::ios::out |
                        //                     std::ios::binary |
                        //                     std::ios::trunc);
                        //ofs.write(e.c_str(), e.size());
                        //ofs.close();
                }
                Poco::File f(t);
                f.moveTo(m_device_json_path);
                m_device_json_updated = false;
                METAX_TRACE(std::string("END ") + __FUNCTION__);
        }
}

void leviathan::metax::backup::
handle_expired_data(const std::string uuid, Poco::JSON::Object::Ptr obj,
                std::vector<std::string>& d_uuids,
                std::vector<std::string>& s_uuids)
{
        poco_assert(nullptr != obj);
        poco_assert(obj->has("expiration_date"));
        Poco::Timestamp e(obj->getValue<Poco::Int64>("expiration_date"));
        Poco::Timestamp n;
        if (0 != e.epochMicroseconds() && n >= e) {
                if (obj->has("is_deleted")) {
                        if (obj->getValue<bool>("is_deleted")) {
                                d_uuids.push_back(uuid);
                        } else {
                                n += Poco::Timespan(30, 0, 0, 0, 0);
                                obj->set("is_deleted", true);
                                obj->set("expiration_date",
                                                n.epochMicroseconds());
                                s_uuids.push_back(uuid);
                                METAX_INFO("Mark " + uuid +
                                                " as deleted in device json.");
                        }
                }
        }
}

void leviathan::metax::backup::
clean_up_expired_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_device_json);
        poco_assert(! m_storage_data.isNull());
        std::vector<std::string> d_uuids;
        std::vector<std::string> s_uuids;
        auto it = m_storage_data->begin();
        for (; it != m_storage_data->end(); ++it) {
                poco_assert(! it->second.isEmpty());
                Poco::JSON::Object::Ptr u =
                        it->second.extract<Poco::JSON::Object::Ptr>();
                poco_assert(nullptr != u);
                if (u->has("expiration_date")) {
                        handle_expired_data(it->first, u, d_uuids, s_uuids);
                }
        }
        post_storage_delete_request_for_uuids(s_uuids);
        for (std::string& s : d_uuids) {
                m_storage_data->remove(s);
        }
        if (! s_uuids.empty() || ! d_uuids.empty()) {
                m_device_json_updated = true;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax::backup::
construct_journal_info_line(Poco::JSON::Object::Ptr obj,
                Poco::Timestamp last_seen, const std::string& uuid)
{
        if (nullptr == obj || ! obj->has("last_updated")
                        || ! obj->has("version")
                        || ! obj->has("is_deleted")) {
                return "";
        }
        std::stringstream ss;
        Poco::Timestamp lu(obj->getValue<Poco::UInt64>("last_updated"));
        if (lu > last_seen) {
                char o = obj->getValue<bool>("is_deleted") ? 'd' : 's';
                ss << uuid << " " << lu.epochMicroseconds() << " "
                        << o << " " << obj->getValue<std::string>("version")
                        << std::endl;
        }
        return ss.str();
}

std::string leviathan::metax::backup::
send_journal_info_by_last_seen_time(Poco::Timestamp last_seen)
{
        METAX_TRACE(__FUNCTION__);
        std::string ss;
        poco_assert(! m_storage_data.isNull());
        auto it = m_storage_data->begin();
        for (; it != m_storage_data->end(); ++it) {
                try {
                        poco_assert(! it->second.isEmpty());
                        Poco::JSON::Object::Ptr u =
                                it->second.extract<Poco::JSON::Object::Ptr>();
                        poco_assert(nullptr != u);
                        ss += construct_journal_info_line(u, last_seen,
                                        it->first);
                } catch (const Poco::Exception& e) {
                        METAX_WARNING(e.displayText());
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return ss;
}

std::string leviathan::metax::backup::
send_journal_info_for_uuids(const std::string& msg, Poco::Timestamp last_seen)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr obj =
                platform::utils::parse_json<Poco::JSON::Object::Ptr>(msg);
        if (! obj->has("uuids")) {
                return "";
        }
        Poco::StringTokenizer st(obj->getValue<std::string>("uuids"), ",");
        poco_assert(! m_storage_data.isNull());
        std::string ss;
        for (auto j : st) {
                try {
                        Poco::JSON::Object::Ptr u =
                                m_storage_data->getObject(j);
                        if (nullptr == u) {
                                continue;
                        }
                        ss += construct_journal_info_line(u, last_seen, j);
                } catch (const Poco::Exception& e) {
                        METAX_WARNING(e.displayText());
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return ss;
}

void leviathan::metax::backup::
handle_add_in_device_json(const kernel_backup_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_device_json);
        poco_assert(! m_storage_data.isNull());
        Poco::JSON::Object::Ptr o = new Poco::JSON::Object;
        o->set("version", in.data_version);
        o->set("is_deleted", in.is_deleted);
        o->set("last_updated", in.last_updated);
        o->set("expiration_date", in.expire_date);
        m_storage_data->set(in.uuid.toString(), o);
        m_device_json_updated = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_add_in_device_json(const backup_user_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        if (0 == in.last_updated) {
                return;
        }
        poco_assert(nullptr != m_device_json);
        poco_assert(! m_storage_data.isNull());
        Poco::JSON::Object::Ptr o = new Poco::JSON::Object;
        o->set("version", in.data_version);
        o->set("is_deleted", false);
        o->set("last_updated", in.last_updated);
        o->set("expiration_date", 0);
        m_storage_data->set(in.uuid.toString(), o);
        m_device_json_updated = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_kernel_delete_from_device_json(const Poco::UUID& uuid,
                uint32_t version, Poco::UInt64 last_updated)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_device_json);
        poco_assert(! m_storage_data.isNull());
        Poco::JSON::Object::Ptr obj =
                m_storage_data->getObject(uuid.toString());
        if (obj.isNull()) {
                return;
        }
        obj->set("is_deleted", true);
        obj->set("version", version);
        obj->set("last_updated", last_updated);
        Poco::Timestamp n;
        n += Poco::Timespan(30, 0, 0, 0, 0);
        obj->set("expiration_date", n.epochMicroseconds());
        m_device_json_updated = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
update_info_for_deleted_uuid(const std::string& uuid,
                const std::string last_updated,
                const std::string dversion)
{
        METAX_TRACE(__FUNCTION__);
        using U = platform::utils;
        uint32_t v = U::from_string<uint32_t>(dversion);
        Poco::UUID u(uuid);
        Poco::UInt64 lu = U::from_string<Poco::UInt64>(last_updated);
        handle_kernel_delete_from_device_json(u, v, lu);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::backup::
should_get_update(const std::string& uuid, const std::string& version,
                const std::string& last_updated)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(! m_storage_data.isNull());
        poco_assert(m_storage_data->has(uuid));
        using U = platform::utils;
        Poco::Timestamp pu = U::from_string<Poco::UInt64>(last_updated);
        uint32_t pv = U::from_string<uint32_t>(version);
        Poco::JSON::Object::Ptr obj = m_storage_data->getObject(uuid);
        poco_assert(! obj.isNull());
        uint32_t ov = obj->getValue<uint32_t>("version");
        if (ov < pv) {
                return true;
        } else if (ov == pv) {
                Poco::Timestamp ou =
                        obj->getValue<Poco::UInt64>("last_updated");
                return ou < pu;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return false;
}

void leviathan::metax::backup::
handle_kernel_get_journal_info(const kernel_backup_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != in.message.get());
        std::string l;
        std::vector<std::string> d;
        std::istringstream iss(in.message.get());
        Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
        while (std::getline(iss, l)) {
                Poco::StringTokenizer st(l, " ");
                poco_assert(4 == st.count());
                if ("d" == st[2]) {
                        update_info_for_deleted_uuid(st[0], st[1], st[3]);
                        d.push_back(st[0]);
                        post_kernel_notify_delete(st[0], platform::utils::
                                        from_string<Poco::UInt64>(st[3]));
                } else if ("s" == st[2]) {
                        if (! m_storage_data->has(st[0])) {
                                if (1 == in.sync_class) {
                                        obj->set(st[0], 's');
                                }
                        } else if (should_get_update(st[0], st[3], st[1])) {
                                obj->set(st[0], 'u');
                        }
                        post_storage_remove_from_cache_request(st[0]);
                } else {
                        // improve the code organization
                        poco_assert(false);
                }
        }
        post_storage_delete_request_for_uuids(d);
        post_kernel_send_uuids(obj, in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
post_kernel_notify_delete(const std::string& uuid, Poco::UInt64 v)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out = *kernel_tx;
        out.cmd = metax::notify_delete;
        out.data_version = v;
        out.uuid.parse(uuid);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
post_storage_remove_from_cache_request(const std::string& uuid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *cache_tx;
        out.cmd = metax::delete_from_cache;
        out.uuid.parse(uuid);
        cache_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
post_kernel_send_uuids(Poco::JSON::Object::Ptr obj, ID32 rid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out = *kernel_tx;
        out.cmd = metax::send_uuids;
        out.request_id = rid;
        out.sync_uuids = obj;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_kernel_dump_user_info(const kernel_backup_package& in)
{
        METAX_TRACE(__FUNCTION__);
        if (! in.key.empty() && ! in.iv.empty()) {
                initialize_key(in.key, in.iv);
                m_device_json_updated = true;
        }
        save_device_json();
        kernel_backup_package& out = *kernel_tx;
        out.cmd = metax::save_confirm;
        out.request_id = in.request_id;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
post_storage_delete_request_for_uuids(const std::vector<std::string>& uuids)
{
        METAX_TRACE(__FUNCTION__);
        if (uuids.empty()) {
                return;
        }
        kernel_storage_package& out = *storage_tx;
        out.cmd = metax::del_uuids;
        std::string s = Poco::cat(std::string(","), uuids.begin(), uuids.end());
        METAX_INFO("Delete uuids from device json: " + s);
        out.set_payload(s);
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_kernel_send_journal_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_backup_package& in = *kernel_rx;
        poco_assert(metax::send_journal_info == in.cmd);
        std::string j;
        if (nullptr == in.message.get()) {
                j = send_journal_info_by_last_seen_time(in.time);
        } else {
                poco_assert(nullptr != in.message.get());
                try {
                        j = send_journal_info_for_uuids(in.message.get(),
                                        in.time);
                } catch (...) {
                        std::string w = "Received invalid sync message: ";
                        METAX_WARNING(w + in.message.get());
                }
        }
        METAX_DEBUG("Requested journal info :\n" + j);
        kernel_backup_package& out = *kernel_tx;
        out.cmd = metax::get_journal_info;
        out.request_id = in.request_id;
        out.set_payload(j);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_router_input()
{
        if (! router_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        (*kernel_tx) = (*router_rx);
        kernel_tx.commit();
        router_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
handle_timer_expiration()
{
        if (m_timeout) {
                METAX_TRACE(__FUNCTION__);
                save_device_json();
                m_timeout = false;
                METAX_TRACE(std::string("END ") + __FUNCTION__);
        }
}

void leviathan::metax::backup::
handle_clean_up_time_expiration()
{
        if (m_clean_up_timeout && nullptr != m_device_json) {
                METAX_TRACE(__FUNCTION__);
                clean_up_expired_data();
                m_clean_up_timeout = false;
                METAX_TRACE(std::string("END ") + __FUNCTION__);
        }
}

void leviathan::metax::backup::
wait_for_final_messages()
{
        METAX_TRACE(__FUNCTION__);
        METAX_INFO("Waiting final message from user manager");
        while (true) {
                user_manager_rx.wait();
                if (user_manager_rx.has_data()) {
                        if (metax::finalize == (*user_manager_rx).cmd) {
                                handle_add_in_device_json((*user_manager_rx));
                                break;
                        }
                        user_manager_rx.consume();
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::backup::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        while (true) {
                // wait to read data
                if (!wait()) {
                        break;
                }
                handle_config_input();
                handle_kernel_input();
                handle_router_input();
                handle_storage_input();
                handle_user_manager_input();
                handle_timer_expiration();
                handle_clean_up_time_expiration();
        }
        m_clean_up_timer.stop();
        m_timer.stop();
        while (kernel_rx.has_data()) {
                handle_kernel_input();
        }
        wait_for_final_messages();
        save_device_json();
        finish();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        std::terminate();
} catch (...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

leviathan::metax::backup::
backup()
        : platform::csp::task("backup", Poco::Logger::get("metax.backup"))
        , kernel_rx(this)
        , router_rx(this)
        , config_rx(this)
        , storage_rx(this)
        , storage_writer_rx(this)
        , cache_rx(this)
        , user_manager_rx(this)
        , kernel_tx(this)
        , router_tx(this)
        , config_tx(this)
        , storage_tx(this)
        , storage_writer_tx(this)
        , cache_tx(this)
        , m_device_json_path()
        , m_device_json_key_path()
        , m_device_json(nullptr)
        , m_storage_data(nullptr)
        , m_device_json_updated(false)
        , m_timer()
        , m_timeout(false)
        , m_clean_up_timer(60000, 86400000) // 1min, 1day
        , m_clean_up_timeout(false)
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::backup::
~backup()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

