/**
 * @file src/metax/user_manager.cpp
 *
 * @brief Implementation of the class @ref
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

// Headers from this project
#include "user_manager.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Base64Decoder.h>
#include <Poco/Crypto/CryptoStream.h>
#include <Poco/FileStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/ThreadPool.h>

// Headers from standard libraries
#include <fstream>

void leviathan::metax::user_manager::
create_cipher(Poco::JSON::Object::Ptr info)
try {
        METAX_TRACE(__FUNCTION__);
        namespace C = Poco::Crypto;
        std::string i = info->getValue<std::string>("aes_iv");
        std::string k = info->getValue<std::string>("aes_key");
        std::string iv = platform::utils::base64_decode(i);
        std::string key = platform::utils::base64_decode(k);
        C::CipherKey::ByteVec v_iv(iv.begin(), iv.end());
        C::CipherKey::ByteVec key_vec(key.begin(), key.end());
        C::CipherKey aes_key(m_enc_type, key_vec, v_iv);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        m_user_json_cipher = factory.createCipher(aes_key);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        std::terminate();
} catch (...) {
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

void leviathan::metax::user_manager::
create_cipher(const std::string& key, const std::string& iv)
try {
        METAX_TRACE(__FUNCTION__);
        namespace C = Poco::Crypto;
        std::string i = platform::utils::base64_decode(iv);
        std::string k = platform::utils::base64_decode(key);
        C::CipherKey::ByteVec v_iv(i.begin(), i.end());
        C::CipherKey::ByteVec key_vec(k.begin(), k.end());
        C::CipherKey aes_key(m_enc_type, key_vec, v_iv);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        m_user_json_cipher = factory.createCipher(aes_key);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        std::terminate();
} catch (...) {
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

std::string leviathan::metax::user_manager::
decrypt_user_json()
{
        METAX_TRACE(__FUNCTION__);
        std::ostringstream decrypt_sink;
        Poco::FileInputStream decrypt_source(m_user_json_path, std::ios::in | std::ios::binary);
        Poco::Crypto::CryptoInputStream decryptor(decrypt_source,
                        m_user_json_cipher->createDecryptor());
        Poco::StreamCopier::copyStream(decryptor, decrypt_sink);
        decrypt_source.close();
        std::string s = decrypt_sink.str();
        if (s.empty()) {
                throw Poco::Exception("Couldn't decrypt user json: "
                                                        + m_user_json_path);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return s;
}

void leviathan::metax::user_manager::
parse_user_json_from_string(const std::string& uj)
{
        METAX_TRACE(__FUNCTION__);
        using O = Poco::JSON::Object;
        m_user_json = platform::utils::parse_json<O::Ptr>(uj);
        if (! m_user_json->has("version")) {
                m_user_json->set("version", 1);
        }
        if (! m_user_json->has("last_updated")) {
                Poco::Timestamp now;
                m_user_json->set("last_updated", now.epochMicroseconds());
        }
        validate_user_json();
        m_owned_data = m_user_json->getObject("owned_data");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
validate_user_json()
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr == m_user_json || ! m_user_json->has("protocol_version")
                                   || ! m_user_json->has("owned_data")) {
                throw Poco::Exception("User json has incorrect format");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
generate_user_json()
{
        METAX_TRACE(__FUNCTION__);
        m_user_json = new Poco::JSON::Object;
        m_user_json->set("protocol_version", 1);
        m_user_json->set("version", 1);
        m_owned_data = new Poco::JSON::Object;
        m_user_json->set("owned_data", m_owned_data);
        m_user_json->set("foreign_data", Poco::JSON::Object());
        Poco::Timestamp now;
        m_user_json->set("last_updated", now.epochMicroseconds());
        m_should_save_user_json = true;
        m_is_user_json_saved = false;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
save_in_file(const std::string c)
{
        std::string t = m_user_json_path + ".tmp";
        std::ofstream ofs(t, std::ofstream::binary);
        ofs.write(c.c_str(), c.size());
        ofs.close();
        Poco::File f(t);
        f.moveTo(m_user_json_path);
}

std::string leviathan::metax::user_manager::
save_user_json(metax::command cmd)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json);
        poco_assert(nullptr != m_user_json_cipher);
        Poco::Timestamp now;
        Poco::UInt64 lu = now.epochMicroseconds();
        m_user_json->set("last_updated", lu);
        std::ostringstream oss;
        m_user_json->stringify(oss);
        std::string euj = m_user_json_cipher->encryptString(oss.str());
        save_in_file(euj);
        update_device_json(cmd);
        if (m_is_user_json_saved) {
                backup_user_json(euj, metax::update, lu);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return oss.str();
}

void leviathan::metax::user_manager::
send_user_json_to_kernel(const std::string& uj)
{
        METAX_TRACE(__FUNCTION__);
        (*kernel_tx).set_payload(uj);
        (*kernel_tx).cmd = metax::user_json;
        (*kernel_tx).uuid = Poco::UUID(m_user_json_uuid);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
request_user_json_from_network()
{
        METAX_TRACE(__FUNCTION__);
        kernel_user_manager_package& out = *kernel_tx;
        out.cmd = metax::get;
        out.uuid = Poco::UUID(m_user_json_uuid);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
process_kernel_connected_request()
{
        METAX_TRACE(__FUNCTION__);
        if (m_should_get_user_json) {
                request_user_json_from_network();
                m_should_get_user_json = false;
        } else if (m_should_save_user_json) {
                handle_kernel_save_fail();
                m_should_save_user_json = false;
        } else {
                kernel_user_manager_package& out = (*kernel_tx);
                out.cmd = metax::on_update_register;
                out.uuid.parse(m_user_json_uuid);
                kernel_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
backup_user_json(const std::string& euj, metax::command cmd, Poco::UInt64 lu)
{
        METAX_TRACE(__FUNCTION__);
        (*kernel_tx).set_payload(euj);
        (*kernel_tx).cmd = cmd;
        (*kernel_tx).last_updated = lu;
        (*kernel_tx).uuid = Poco::UUID(m_user_json_uuid);
        (*kernel_tx).data_version =
                m_user_json->getValue<Poco::UInt64>("version");
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
update_device_json(metax::command cmd)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json);
        backup_user_manager_package& bout = *backup_tx;
        bout.cmd = cmd;
        bout.data_version = m_user_json->getValue<Poco::UInt64>("version");
        bout.last_updated =
                m_user_json->getValue<Poco::UInt64>("last_updated");;
        bout.uuid = Poco::UUID(m_user_json_uuid);
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
init_user_json(const backup_cfg_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != in.user_json_info_ptr);
        poco_assert(in.user_json_info_ptr->has("uuid"));
        create_cipher(in.user_json_info_ptr);
        m_user_json_uuid = in.user_json_info_ptr->getValue<std::string>("uuid");
        poco_assert(! m_user_json_uuid.empty());
        if (in.is_generated) {
                m_user_json_path = m_storage_path + '/' + m_user_json_uuid;
                generate_user_json();
                std::string ujs = save_user_json(metax::save);
                send_user_json_to_kernel(ujs);
        } else {
                process_user_json_get();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
process_user_json_get()
{
        METAX_TRACE(__FUNCTION__);
        m_user_json_path = m_storage_path + '/' + m_user_json_uuid;
        if (Poco::File(m_user_json_path).exists()) {
                std::string uj = decrypt_user_json();
                parse_user_json_from_string(uj);
                send_user_json_to_kernel(uj);
        } else {
                m_should_get_user_json = true;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
start_timer()
{
        METAX_TRACE(__FUNCTION__);
        if (0 < m_metadata_dump_timer) {
                Poco::TimerCallback<user_manager>
                        on_timer_callback(*this, &user_manager::on_timer);
                m_timer.setPeriodicInterval(
                                        1000 * m_metadata_dump_timer);
                m_timer.setStartInterval(1000 * m_metadata_dump_timer);
                m_timer.start(on_timer_callback,
                                        Poco::ThreadPool::defaultPool());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
on_timer(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        m_timeout = true;
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_timer_expiration()
{
        if (m_timeout) {
                METAX_TRACE(__FUNCTION__);
                if (m_user_json_updated) {
                        update_version();
                        save_user_json();
                        m_user_json_updated = false;
                }
                m_timeout = false;
                METAX_TRACE(std::string("END ") + __FUNCTION__);
        }
}

void leviathan::metax::user_manager::
handle_kernel_save()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_user_manager_package& in = *kernel_rx;
        Poco::JSON::Object::Ptr s = new Poco::JSON::Object();
        s->set("aes_key", in.key);
        s->set("aes_iv", in.iv);
        s->set("copy_count", in.copy_count);
        m_owned_data->set(in.uuid.toString(), s);
        m_user_json_updated = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_delete()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_user_manager_package& in = *kernel_rx;
        m_owned_data->remove(in.uuid.toString());
        m_user_json_updated = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
update_version()
{
        METAX_TRACE(__FUNCTION__);
        Poco::UInt64 v = m_user_json->getValue<Poco::UInt64>("version");
        m_user_json->set("version", ++v);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
process_kernel_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_user_manager_package& in = *kernel_rx;
        switch (in.cmd) {
                case metax::save: {
                        handle_kernel_save();
                        break;
                } case metax::del: {
                        handle_kernel_delete();
                        break;
                } case metax::dump_user_info: {
                        handle_kernel_dump_user_info(in);
                        break;
                } case metax::update: {
                        handle_kernel_update();
                        break;
                } case metax::save_data: {
                        parse_recieved_user_json(in);
                        break;
                } case metax::connected: {
                        process_kernel_connected_request();
                        break;
                } case metax::failed: {
                        request_user_json_from_network();
                        break;
                } case metax::save_fail: {
                        handle_kernel_save_fail();
                        break;
                } case metax::save_confirm: {
                        handle_kernel_save_confirm();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_save_confirm()
{
        METAX_TRACE(__FUNCTION__);
        m_is_user_json_saved = true;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_save_fail()
{
        METAX_TRACE(__FUNCTION__);
        if (m_user_json_updated) {
                update_version();
                save_user_json();
                m_user_json_updated = false;
        }
        std::ostringstream oss;
        m_user_json->stringify(oss);
        std::string euj = m_user_json_cipher->encryptString(oss.str());
        backup_user_json(euj, metax::save,
                        m_user_json->getValue<Poco::UInt64>("last_updated"));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
parse_recieved_user_json(const kernel_user_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json_cipher);
        std::string str(in.message.get(), in.size);
        save_in_file(str);
        std::string uj = m_user_json_cipher->decryptString(str);
        parse_user_json_from_string(uj);
        send_user_json_to_kernel(uj);
        // Register on user json updates.
        kernel_user_manager_package& out = (*kernel_tx);
        out.cmd = metax::on_update_register;
        out.uuid.parse(m_user_json_uuid);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_dump_user_info(const kernel_user_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        if (! in.key.empty() && ! in.iv.empty()) {
                create_cipher(in.key, in.iv);
                m_user_json_updated = true;
        }
        if (m_user_json_updated) {
                update_version();
                save_user_json();
                m_user_json_updated = false;
        }
        kernel_user_manager_package& out = *kernel_tx;
        out.cmd = metax::save_confirm;
        out.request_id = in.request_id;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_update()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_user_manager_package& in = *kernel_rx;
        Poco::UInt64 v = m_user_json->getValue<Poco::UInt64>("version");
        if (v < in.data_version) {
                request_user_json_from_network();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
handle_kernel_input()
{
        if (! kernel_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
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

void leviathan::metax::user_manager::
handle_config_input()
try {
        if (! config_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        const backup_cfg_package& in = *config_rx;
        m_storage_path = in.metadata_path;
        m_metadata_dump_timer = in.periodic_interval;
        Poco::File f(m_storage_path);
        f.createDirectories();
        poco_assert(f.exists());
        init_user_json(in);
        start_timer();
        config_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (const Poco::Exception& e) {
        METAX_FATAL(e.displayText());
        (*kernel_tx).cmd = metax::key_init_fail;
        kernel_tx.commit();
        config_rx.consume();
} catch (...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhandled exception.");
        (*kernel_tx).cmd = metax::key_init_fail;
        kernel_tx.commit();
        config_rx.consume();
}

void leviathan::metax::user_manager::
handle_user_json_final_save()
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr == m_user_json) {
                backup_user_manager_package& out = *backup_tx;
                out.cmd = metax::finalize;
                out.last_updated = 0;
                backup_tx.commit();
                return;
        }
        if (m_user_json_updated) {
                update_version();
                save_user_json(metax::finalize);
        } else {
                update_device_json(metax::finalize);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::user_manager::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        while (true) {
                if (!wait()) {
                        break;
                }
                handle_config_input();
                handle_kernel_input();
                handle_timer_expiration();
        }
        m_timer.stop();
        while (kernel_rx.has_data()) {
                handle_kernel_input();
        }
        handle_user_json_final_save();
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

leviathan::metax::user_manager::
user_manager()
        : platform::csp::task("user_manager", Poco::Logger::get("metax.user_manager"))
        , kernel_rx(this)
        , config_rx(this)
        , kernel_tx(this)
        , backup_tx(this)
        , m_metadata_dump_timer(5)
        , m_user_json_path()
        , m_user_json_info_path()
        , m_storage_path()
        , m_user_json_uuid()
        , m_user_json(nullptr)
        , m_owned_data(nullptr)
        , m_enc_type("aes256")
        , m_user_json_cipher(nullptr)
        , m_user_json_updated(false)
        , m_should_get_user_json(false)
        , m_should_save_user_json(false)
        , m_is_user_json_saved(true)
        , m_timer()
        , m_timeout(false)
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::user_manager::
~user_manager()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

