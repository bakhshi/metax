
/**
 * @file src/metax/configuration_manager.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::configuration_manager
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "configuration_manager.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/JSON/JSON.h>
#include <Poco/Message.h>
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/AutoPtr.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>

// Headers from standard libraries
#include <string>
#include <sstream>
#include <fstream>

void leviathan::metax::configuration_manager::
handle_key_manager_input()
{
        if (! key_manager_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_key_manager_input();
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
        key_manager_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
process_key_manager_input()
{
        poco_assert(key_manager_rx.has_data());
        switch ((*key_manager_rx).cmd) {
                case metax::initialized_keys: {
                        handle_key_manager_initialized_keys();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
}

void leviathan::metax::configuration_manager::
write_storage_configuration(const metax::storage_cfg_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::write_storage_cfg == in.cmd);
        std::stringstream ss;
        ss << R"({"cache_size":)" << in.cache_size << ","
                << R"("storage_size":)" << in.storage_size << "}";
        std::ofstream ofs(cfg_data.storage_path + "/storage.json");
        std::string s = ss.str();
        ofs.write(s.c_str(), s.size());
        ofs.close();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_cache_input()
{
        if (! cache_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_storage_input(*cache_rx);
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
        cache_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_storage_writer_input()
{
        if (! storage_writer_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_storage_input(*storage_writer_rx);
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
        storage_writer_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_storage_input()
{
        if (! storage_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_storage_input(*storage_rx);
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
        storage_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
process_storage_input(const storage_cfg_package& in)
{
        switch (in.cmd) {
                case metax::write_storage_cfg: {
                        write_storage_configuration(in);
                        break;
                } default: {
                }
        }
}

void leviathan::metax::configuration_manager::
handle_wallet_input()
{
        if (! wallet_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_wallet_input();
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
        wallet_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
process_wallet_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(wallet_rx.has_data());
        std::string s((*wallet_rx).message);
        METAX_INFO("Got message from wallet: " + s);
        (*wallet_tx).set_payload(std::string(name()
                                + " end of the connection "));
        wallet_tx.commit();
        wallet_tx.deactivate();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_router_input()
{
        if (! router_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_router_input();
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
        router_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
process_router_input()
{
        poco_assert(router_rx.has_data());
        std::string s((*router_rx).message);
        METAX_INFO(" Got message from router: " + s);
        (*router_tx).set_payload(std::string(name() + " end of the connection "));
        router_tx.commit();
        router_tx.deactivate();
}

void leviathan::metax::configuration_manager::
send_friend_not_found()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_friend == in.cmd);
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_friend_not_found;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_friend()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_friend == in.cmd);
        poco_assert(0 != in.message.get());
        std::string s = in.message.get();
        std::map<std::string, std::string>::iterator i;
        i = m_friends.find(s);
        if (m_friends.end() == i) {
                return send_friend_not_found();
        } else {
                metax::kernel_cfg_package& out = *kernel_tx;
                out.request_id = in.request_id;
                out.cmd = metax::get_friend_found;
                out.set_payload(i->second);
                kernel_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_add_trusted_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::add_peer == in.cmd);
        poco_assert(nullptr != in.message.get());
        std::string k(in.message.get(), in.size);
        METAX_INFO("Adding trusted peer: " + k);
        platform::utils::trim(k);
        m_md5_engine.update(k);
        m_trusted_peers.insert(std::make_pair(m_md5_engine.digest(),
                                                        peer_info(k, "")));
        std::ofstream ofs(cfg_data.trusted_peers_json);
        if (! ofs.is_open()) {
                std::string e = "unable to write in " +
                        cfg_data.trusted_peers_json + " file";
                return send_add_peer_failed_to_kernel(in.request_id, e);
        }
        std::string s = get_trusted_peer_list();
        ofs.write(s.c_str(), s.size());
        ofs.close();
        send_add_peer_confirm_to_kernel(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_remove_trusted_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::remove_peer == in.cmd);
        poco_assert(nullptr != in.message.get());
        std::string k(in.message.get(), in.size);
        METAX_INFO("Removing trusted peer: " + k);
        platform::utils::trim(k);
        m_md5_engine.update(k);
        m_trusted_peers.erase(m_md5_engine.digest());
        std::ofstream ofs(cfg_data.trusted_peers_json);
        if (! ofs.is_open()) {
                std::string e = "unable to remove trusted peer from " +
                        cfg_data.trusted_peers_json + " file";
                return send_remove_peer_failed_to_kernel(in.request_id, e);
        }
        std::string s = get_trusted_peer_list();
        ofs.write(s.c_str(), s.size());
        ofs.close();
        send_remove_peer_confirm_to_kernel(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_add_friend()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::add_friend == in.cmd);
        poco_assert(nullptr != in.message.get());
        poco_assert(nullptr != in.user_id.message.get());
        std::string n = in.message.get();
        std::string k = in.user_id.message.get();
        METAX_INFO("Adding friend: " + n);
        platform::utils::trim(k);
        m_friends[n] = k;
        std::ofstream ofs(cfg_data.friends_json);
        if (! ofs.is_open()) {
                std::string e = "unable to write in " +
                        cfg_data.friends_json + " file";
                return send_add_friend_failed_to_kernel(in.request_id, n, e);
        }
        std::string s = get_friend_list();
        METAX_NOTICE("ERROR = " + s);
        ofs.write(s.c_str(), s.size());
        ofs.close();
        send_add_friend_confirm_to_kernel(in.request_id, n);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
send_add_peer_confirm_to_kernel(ID32 rid)
{
        metax::kernel_cfg_package& out = *kernel_tx;
        std::string res = R"({"success":"Adding peer completed successfully"})";
        out.request_id = rid;
        out.cmd = metax::add_peer_confirm;
        out.set_payload(res);
        kernel_tx.commit();
}

void leviathan::metax::configuration_manager::
send_add_peer_failed_to_kernel(ID32 rid, const std::string err)
{
        std::string e = "Adding peer failed: " + err;
        METAX_WARNING(e);
        std::string res = R"({"error":")" + e + R"("})";
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = rid;
        out.cmd = metax::add_peer_failed;
        out.set_payload(res);
        kernel_tx.commit();
}

void leviathan::metax::configuration_manager::
send_remove_peer_confirm_to_kernel(ID32 rid)
{
        metax::kernel_cfg_package& out = *kernel_tx;
        std::string res = R"({"success":"Removing peer completed successfully"})";
        out.request_id = rid;
        out.cmd = metax::remove_peer_confirm;
        out.set_payload(res);
        kernel_tx.commit();
}

void leviathan::metax::configuration_manager::
send_remove_peer_failed_to_kernel(ID32 rid, const std::string err)
{
        std::string e = "Removing peer failed: " + err;
        METAX_WARNING(e);
        std::string res = R"({"error":")" + e + R"("})";
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = rid;
        out.cmd = metax::remove_peer_failed;
        out.set_payload(res);
        kernel_tx.commit();
}

void leviathan::metax::configuration_manager::
send_add_friend_confirm_to_kernel(ID32 rid, const std::string& n)
{
        metax::kernel_cfg_package& out = *kernel_tx;
        std::string res = R"({"success":"Adding friend )" + n
                        + R"( completed successfully."})";
        out.request_id = rid;
        out.cmd = metax::add_friend_confirm;
        out.set_payload(res);
        kernel_tx.commit();
}

void leviathan::metax::configuration_manager::
send_add_friend_failed_to_kernel(ID32 rid, const std::string& n,
                const std::string err)
{
        std::string e = "Adding friend " + n + " failed: " + err;
        METAX_WARNING(e);
        std::string res = R"({"error":")" + e + R"("})";
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = rid;
        out.cmd = metax::add_friend_failed;
        out.set_payload(res);
        kernel_tx.commit();
}

std::string leviathan::metax::configuration_manager::
get_trusted_peer_list()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr tpl = new Poco::JSON::Object;
        auto i = m_trusted_peers.begin();
        for (; i != m_trusted_peers.end(); ++i) {
                Poco::JSON::Object::Ptr o = new Poco::JSON::Object();
                o->set("device_key", i->second.device_key);
                o->set("user_key", i->second.user_key);
                tpl->set(Poco::DigestEngine::digestToHex(i->first), o);
        }
        std::ostringstream oss;
        tpl->stringify(oss);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return oss.str();
}

std::string leviathan::metax::configuration_manager::
get_friend_list()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Array::Ptr arr = new Poco::JSON::Array;
        auto i = m_friends.begin();
        for (; i != m_friends.end(); ++i) {
                Poco::JSON::Object::Ptr o = new Poco::JSON::Object();
                o->set("name", i->first);
                o->set("public_key", i->second);
                arr->add(o);
        }
        std::ostringstream oss;
        arr->stringify(oss);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return oss.str();
}

void leviathan::metax::configuration_manager::
handle_kernel_get_trusted_peer_list()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_peer_list == in.cmd);
        METAX_INFO("Getting trusted peer list");
        std::string r = get_trusted_peer_list();
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_peer_list_response;
        out.set_payload(r);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_friend_list()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_friend_list == in.cmd);
        METAX_INFO("Getting friend list");
        std::string r = get_friend_list();
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_friend_list_response;
        out.set_payload(r);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_public_key()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_public_key == in.cmd);
        std::ifstream ifs(cfg_data.user_key_path + "/public.pem");
        METAX_INFO("Getting public key");
        std::string pk = "";
        if (ifs.is_open()) {
                pk.assign((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));
                platform::utils::trim(pk);
        }
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_public_key_response;
        out.set_payload(pk);
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_online_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_online_peers == in.cmd);
        metax::link_cfg_package& out = *link_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_online_peers;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_pairing_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::get_pairing_peers == in.cmd);
        metax::link_cfg_package& out = *link_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_pairing_peers;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_get_metax_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        METAX_INFO("Getting metax info");
        poco_assert(metax::get_metax_info == in.cmd);
        std::string uk = "";
        std::string dk = "";
        std::string ui = "";
        std::string di = "";
        std::ifstream uifs(cfg_data.user_key_path + "/public.pem");
        if (uifs.is_open()) {
                uk.assign((std::istreambuf_iterator<char>(uifs)),
                                (std::istreambuf_iterator<char>()));
                platform::utils::trim(uk);
        }
        std::ifstream difs(cfg_data.device_key_path + "/public.pem");
        if (difs.is_open()) {
                dk.assign((std::istreambuf_iterator<char>(difs)),
                                (std::istreambuf_iterator<char>()));
                platform::utils::trim(dk);
        }
        using U = platform::utils;
        using O = Poco::JSON::Object;
        std::ifstream muifs(cfg_data.metax_user_info_path);
        if (muifs.is_open()) {
                try {
                        O::Ptr muij = U::parse_json<O::Ptr>(muifs);
                        if (muij->has("user_info_uuid")) {
                                ui = muij->getValue<std::string>(
                                                        "user_info_uuid");
                        }
                } catch (const Poco::Exception& e) {
                        METAX_ERROR("Invalid metax user info json");
                }
        }
        std::ifstream mdifs(cfg_data.metax_device_info_path);
        if (mdifs.is_open()) {
                try {
                        O::Ptr mdij = U::parse_json<O::Ptr>(mdifs);
                        if (mdij->has("device_info_uuid")) {
                                di = mdij->getValue<std::string>(
                                                        "device_info_uuid");
                        }
                } catch (const Poco::Exception& e) {
                        METAX_ERROR("Invalid metax device info json");
                }
        }
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_metax_info_response;
        Poco::JSON::Object::Ptr mi = new Poco::JSON::Object;
        mi->set("user_info_uuid", ui);
        mi->set("device_info_uuid", di);
        mi->set("user_public_key", uk);
        mi->set("device_public_key", dk);
        std::ostringstream ostr;
        mi->stringify(ostr);
        out.set_payload(ostr.str());
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_set_metax_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        std::string u(in.message.get(), in.size);
        METAX_INFO("Setting metax info");
        poco_assert(metax::set_metax_info == in.cmd);
        metax::kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        using U = platform::utils;
        using O = Poco::JSON::Object;
        std::ifstream ifs(cfg_data.metax_user_info_path);
        if (ifs.is_open()) {
                try {
                        O::Ptr muij = U::parse_json<O::Ptr>(ifs);
                        if ("" !=
                                muij->getValue<std::string>("user_info_uuid")) {
                                out.set_payload(std::string(
                                                "Metax info already exists"));
                                out.cmd = metax::set_metax_info_fail;
                                kernel_tx.commit();
                                return;
                        }
                } catch (const Poco::Exception& e) {
                        METAX_WARNING("Invalid metax user info json,"
                                        " overwriting");
                }
        }
        ifs.close();
        std::ofstream ofs(cfg_data.metax_user_info_path);
        if (ofs.is_open()) {
                ofs << R"({"user_info_uuid":")" << u << R"("})";
                out.cmd = metax::set_metax_info_ok;
                kernel_tx.commit();
        } else {
                out.set_payload(std::string("Unable to save user info"));
                out.cmd = metax::set_metax_info_fail;
                kernel_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_reconnect_to_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::reconnect_to_peers == in.cmd);
        (*link_tx).cmd = in.cmd;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_start_pairing()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::start_pairing == in.cmd);
        metax::link_cfg_package& out = *link_tx;
        out.request_id = in.request_id;
        out.cmd = in.cmd;
        poco_assert(nullptr != in.message.get());
        std::string s = in.message.get();
        out.set_payload(s);
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_cancel_pairing()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::cancel_pairing == in.cmd);
        metax::link_cfg_package& out = *link_tx;
        out.request_id = in.request_id;
        out.cmd = metax::cancel_pairing;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_kernel_request_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        poco_assert(metax::request_keys == in.cmd);
        metax::link_cfg_package& out = *link_tx;
        out.request_id = in.request_id;
        out.cmd = in.cmd;
        poco_assert(nullptr != in.message.get());
        std::string s = in.message.get();
        out.set_payload(s);
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
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

void leviathan::metax::configuration_manager::
process_kernel_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const metax::kernel_cfg_package& in = *kernel_rx;
        switch (in.cmd) {
                case metax::add_peer: {
                        handle_kernel_add_trusted_peer();
                        break;
                } case metax::remove_peer: {
                        handle_kernel_remove_trusted_peer();
                        break;
                } case metax::get_peer_list: {
                        handle_kernel_get_trusted_peer_list();
                        break;
                } case metax::add_friend: {
                        handle_kernel_add_friend();
                        break;
                } case metax::get_friend_list: {
                        handle_kernel_get_friend_list();
                        break;
                } case metax::get_friend: {
                        handle_kernel_get_friend();
                        break;
                } case metax::get_public_key: {
                        handle_kernel_get_public_key();
                        break;
                } case metax::get_online_peers: {
                        handle_kernel_get_online_peers();
                        break;
                } case metax::get_pairing_peers: {
                        handle_kernel_get_pairing_peers();
                        break;
                } case metax::get_metax_info: {
                        handle_kernel_get_metax_info();
                        break;
                } case metax::set_metax_info: {
                        handle_kernel_set_metax_info();
                        break;
                } case metax::reconnect_to_peers: {
                        handle_kernel_reconnect_to_peers();
                        break;
                } case metax::start_pairing: {
                        handle_kernel_start_pairing();
                        break;
                } case metax::cancel_pairing: {
                        handle_kernel_cancel_pairing();
                        break;
                } case metax::request_keys: {
                        handle_kernel_request_keys();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_input()
{
        if (! link_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_link_input();
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
        link_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_send_notification_to_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::send_to_peer == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.cmd = in.cmd;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_get_online_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::get_online_peers_response == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_online_peers_response;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_get_pairing_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::get_pairing_peers_response == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_pairing_peers_response;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_get_generated_code()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::get_generated_code == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::get_generated_code;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_start_pairing_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::start_pairing_fail == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = metax::start_pairing_fail;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_request_keys_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        poco_assert(metax::request_keys_fail == in.cmd);
        kernel_cfg_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.cmd = in.cmd;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
send_peer_verify_response(ID32 pid,
                        platform::default_package di, metax::command cmd)
{
        METAX_TRACE(__FUNCTION__);
        metax::link_cfg_package& out = *link_tx;
        out.peer_id = pid;
        out.device_id = di;
        out.cmd = cmd;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
handle_link_verify_peer_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        std::string pbk(in.message.get(), in.size);
        auto it = m_trusted_peers.find(Poco::DigestEngine::Digest(
                        in.device_id.message.get(),
                        in.device_id.message.get() + in.device_id.size));
        if (m_trusted_peers.end() == it) {
                send_peer_verify_response(in.peer_id, in.device_id,
                                                metax::peer_not_verified);
                return;
        }
        if ("" == it->second.user_key) {
                it->second.user_key = pbk;
                std::ofstream ofs(cfg_data.trusted_peers_json);
                if (ofs.is_open()) {
                        std::string s = get_trusted_peer_list();
                        ofs.write(s.c_str(), s.size());
                        ofs.close();
                }
        } else if (pbk != it->second.user_key) {
                send_peer_verify_response(in.peer_id, in.device_id,
                                                metax::peer_not_verified);
                return;
        }
        send_peer_verify_response(in.peer_id, in.device_id,
                                                metax::peer_verified);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
process_link_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const metax::link_cfg_package& in = *link_rx;
        switch (in.cmd) {
                case metax::send_to_peer: {
                        handle_link_send_notification_to_peer();
                        break;
                } case metax::get_online_peers_response: {
                        handle_link_get_online_peers_response();
                        break;
                } case metax::get_pairing_peers_response: {
                        handle_link_get_pairing_peers_response();
                        break;
                } case metax::get_generated_code: {
                        handle_link_get_generated_code();
                        break;
                } case metax::start_pairing_fail: {
                        handle_link_start_pairing_fail();
                        break;
                } case metax::request_keys_fail: {
                        handle_link_request_keys_fail();
                        break;
                } case metax::verify_peer: {
                        handle_link_verify_peer_request();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
initialize_user_manager()
{
        backup_cfg_package& out = *user_manager_tx;
        out.cmd = metax::backup_cfg;
        out.metadata_path = cfg_data.storage_path;
        out.user_json_info_ptr = (*key_manager_rx).user_json_info_ptr;
        out.is_generated = (*key_manager_rx).is_generated;
        out.periodic_interval = cfg_data.metadata_dump_timer;
        user_manager_tx.commit();
        user_manager_tx.deactivate();
}

void leviathan::metax::configuration_manager::
initialize_link_layer()
{
        using U = platform::utils;
        // Post device key path to link for generating device id.
        (*link_tx).cmd = metax::device_id;
        Poco::JSON::Array::Ptr arr = new Poco::JSON::Array;
        arr->add(cfg_data.device_key_path);
        arr->add(cfg_data.user_key_path + "/public.pem");
        arr->add(cfg_data.peers_rating);
        arr->add(U::to_string(cfg_data.peers_default_rating));
        arr->add(U::to_string(cfg_data.sync_class));
        arr->add(cfg_data.user_key_path);
        arr->add(cfg_data.user_json_info);
        arr->add(cfg_data.use_ssl);
        arr->add(cfg_data.peer_verification_mode);
        std::ostringstream ss;
        arr->stringify(ss);
        (*link_tx).set_payload(ss.str());
        link_tx.commit();
        // Post port number.
        (*link_tx).cmd = metax::bind;
        (*link_tx).set_payload(U::to_string(cfg_data.port));
        link_tx.commit();
        // Post connect request for each peer.
        auto b = cfg_data.peers.begin();
        auto e = cfg_data.peers.end();
        //uint16_t rid = 0;
        for(; b != e; ++b) {
                (*link_tx).cmd = metax::connect;
                (*link_tx).set_payload((b->first));
                (*link_tx).peer_id = b->second;
                //(*link_tx).request_id = rid++;
                link_tx.commit();
        }
        link_tx.deactivate();
}

void leviathan::metax::configuration_manager::
initialize_key_manager()
{
        // Sending device and user key paths to key_manager
        (*key_manager_tx).set_payload(cfg_data.device_key_path +
                                ' ' + cfg_data.user_key_path +
                                ' ' + cfg_data.user_json_info);
        (*key_manager_tx).cmd = metax::key_path;
        key_manager_tx.commit();
}

void leviathan::metax::configuration_manager::
initialize_storage()
{
        METAX_TRACE(__FUNCTION__);
        // Send storage path to storage.
        storage_cfg_package& out = *storage_tx;
        out.cmd = metax::storage_cfg;
        out.set_payload(cfg_data.storage_path);
        set_storage_configuration(out);
        out.cache_size_limit_in_gb = cfg_data.cache_size_limit_in_gb;
        out.cache_age = cfg_data.cache_age;
        out.size_limit_in_gb = cfg_data.size_limit_in_gb;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
initialize_backup()
{
        backup_cfg_package& out = *backup_tx;
        out.cmd = metax::backup_cfg;
        out.metadata_path = cfg_data.device_json;
        out.user_json_info_ptr = (*key_manager_rx).user_json_info_ptr;
        out.periodic_interval = cfg_data.metadata_dump_timer;
        out.set_payload(platform::utils::read_file_content(
                                        cfg_data.user_json_info));
        backup_tx.commit();
}

void leviathan::metax::configuration_manager::
initialize_kernel()
{
        METAX_TRACE(__FUNCTION__);
        (*kernel_tx).cmd = metax::kernel_cfg;
        (*kernel_tx).storage_class = cfg_data.storage_class;
        (*kernel_tx).sync_class = cfg_data.sync_class;
        (*kernel_tx).enable_livestream_hosting =
                                cfg_data.enable_livestream_hosting;
        (*kernel_tx).data_copy_count = cfg_data.data_copy_count;
        (*kernel_tx).router_save_queue_size =
                        cfg_data.router_save_queue_size;
        (*kernel_tx).router_get_queue_size =
                        cfg_data.router_get_queue_size;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
initialize_router()
{
        (*router_tx).cmd = metax::router_config;
        (*router_tx).max_hops = cfg_data.max_hops;
        (*router_tx).peer_response_wait_time = cfg_data.peer_response_wait_time;
        router_tx.commit();
}

void leviathan::metax::configuration_manager::
handle_key_manager_initialized_keys()
{
        METAX_TRACE(__FUNCTION__);
        initialize_kernel();
        initialize_backup();
        initialize_user_manager();
        initialize_link_layer();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        initialize_key_manager();
        initialize_router();
        initialize_storage();
        while (true) {
                METAX_INFO("WAITING");
                // wait to read data
                if (! wait()) {
                        break;
                }
                handle_key_manager_input();
                handle_storage_input();
                handle_cache_input();
                handle_storage_writer_input();
                handle_wallet_input();
                handle_link_input();
                handle_router_input();
                handle_kernel_input();
        }
        finish();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch(...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhandled exception.");
        std::terminate();
}

void leviathan::metax::configuration_manager::
add_peer_address(const Poco::Util::XMLConfiguration* c,
                        const std::string& pr, const std::string& ad, int g)
{
        if ("peer" == ad.substr(0, 4)) {
                std::string up = c->getString(pr + ad);
                cfg_data.peers.push_back(std::make_pair(up, g));
                METAX_INFO("Peer " + up);
        }
}

void leviathan::metax::configuration_manager::
get_peer_info(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        Poco::Util::XMLConfiguration::Keys peers;
        c->keys("peers", peers);
        for (int i = 0; i < peers.size(); ++i) {
                if ("balancer" == peers[i].substr(0, 8)) {
                        Poco::Util::XMLConfiguration::Keys bp;
                        c->keys("peers." + peers[i], bp);
                        for (auto& j : bp) {
                                add_peer_address(c,
                                        "peers." + peers[i] + ".", j, i);
                        }
                } else {
                        add_peer_address(c, "peers.", peers[i], -1);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_device_key_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        Poco::Path p(Poco::Path::expand(
                                c->getString("device_key_path")));
        if (p.isAbsolute()) {
                cfg_data.device_key_path = p.toString();
        } else {
                cfg_data.device_key_path =
                        m_base_path + p.toString();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() +
                        ". Defaulting to " + cfg_data.device_key_path)
}

void leviathan::metax::configuration_manager::
get_user_key_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        Poco::Path p(Poco::Path::expand(
                                c->getString("user_key_path")));
        if (p.isAbsolute()) {
                cfg_data.user_key_path = p.toString();
        } else {
                cfg_data.user_key_path =
                        m_base_path + p.toString();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() +
                        ". Defaulting to " + cfg_data.user_key_path)
}

void leviathan::metax::configuration_manager::
get_storage_config(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        Poco::Path p(Poco::Path::expand(
                                c->getString("storage.path")));
        if (p.isAbsolute()) {
                cfg_data.storage_path = p.toString();
        } else {
                cfg_data.storage_path =
                        m_base_path + p.toString();
        }
        cfg_data.cache_size_limit_in_gb =
                c->getDouble("storage.cache_size_limit", 5);
        cfg_data.cache_age =
                c->getDouble("storage.cache_age", 2);
        cfg_data.size_limit_in_gb =
                c->getDouble("storage.size_limit", -1);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() +
                        ". Defaulting to " + cfg_data.storage_path)
}

void leviathan::metax::configuration_manager::
get_storage_class(const Poco::Util::XMLConfiguration* c)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        std::string m = "";
        try {
                cfg_data.storage_class = c->getUInt("storage.storage_class");
                if (3 > cfg_data.storage_class) {
                        METAX_TRACE(std::string("END ") + __FUNCTION__);
                        return;
                }

                m = "Invalid storage_class value, should be in [0 - 2]";
        } catch(Poco::RuntimeException& e) {
                m = e.displayText();
        }
        METAX_WARNING(m + ", defaulting storage_class to 2");
        cfg_data.storage_class = 2;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
get_sync_class(const Poco::Util::XMLConfiguration* c)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        std::string m = "";
        try {
                cfg_data.sync_class = c->getUInt("storage.sync_class");
                if (2 > cfg_data.sync_class) {
                        METAX_TRACE(std::string("END ") + __FUNCTION__);
                        return;
                }

                m = "Invalid sync_class value, should be in [0 - 1]";
        } catch(Poco::RuntimeException& e) {
                m = e.displayText();
        }
        METAX_WARNING(m + ", defaulting sync_class to 0");
        cfg_data.sync_class = 0;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
get_enable_livestream_hosting(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        cfg_data.enable_livestream_hosting =
                c->getBool("storage.enable_livestream_hosting");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() + ", defaulting "
                        "enable_livestream_hosting to true");
        cfg_data.enable_livestream_hosting = true;
}

void leviathan::metax::configuration_manager::
get_data_copy_count(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        uint16_t cp = (unsigned short)c->getUInt(
                        "storage.data_copy_count",
                        cfg_data.data_copy_count);
        if (1 <= cp) {
                cfg_data.data_copy_count = cp;
        } else {
                METAX_WARNING("Invalid data copy count : "
                                + platform::utils::to_string(cp));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_metadata_dump_timer(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        uint16_t cp = (unsigned short)c->getUInt(
                        "metadata_dump_timer",
                        cfg_data.metadata_dump_timer);
        cfg_data.metadata_dump_timer = cp;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_router_save_queue_size(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        uint32_t sq = (uint32_t)c->getUInt64(
                        "router.router_save_queue_size",
                        cfg_data.router_save_queue_size);
        if (1 <= sq) {
                cfg_data.router_save_queue_size = sq;
        } else {
                METAX_WARNING("Invalid router save queue size : "
                                + platform::utils::to_string(sq));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() + ". Defaulting to "
                + platform::utils::to_string(cfg_data.router_save_queue_size));
}

void leviathan::metax::configuration_manager::
get_router_get_queue_size(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        uint32_t gq = (uint32_t)c->getUInt64(
                        "router.router_get_queue_size",
                        cfg_data.router_get_queue_size);
        if (1 <= gq) {
                cfg_data.router_get_queue_size = gq;
        } else {
                METAX_WARNING("Invalid router get queue size: "
                                + platform::utils::to_string(gq));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch(Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() + ". Defaulting to "
                + platform::utils::to_string(cfg_data.router_get_queue_size));
}

void leviathan::metax::configuration_manager::
get_trusted_peers_json_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        cfg_data.trusted_peers_json = Poco::Path::expand(c->getString(
                        "trusted_peers_config",
                        Poco::Path::home() + ".leviathan/trusted_peers.json"));
        std::ifstream ifs(cfg_data.trusted_peers_json);
        if (ifs.is_open()) {
                namespace P = leviathan::platform;
                Poco::JSON::Object::Ptr pc =
                        P::utils::parse_json<Poco::JSON::Object::Ptr>(ifs);
                for (auto it = pc->begin(); it != pc->end(); ++it) {
                        try {
                        Poco::JSON::Object::Ptr p =
                                it->second.extract<Poco::JSON::Object::Ptr>();
                        peer_info& pi = m_trusted_peers[
                                Poco::DigestEngine::digestFromHex(it->first)];
                        pi.device_key = p->getValue<std::string>("device_key");
                        pi.user_key = p->getValue<std::string>("user_key");
                        } catch (Poco::Exception& e) {
                                METAX_WARNING(e.displayText());
                        }
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_friends_json_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        cfg_data.friends_json = Poco::Path::expand(c->getString(
                        "friends_config",
                        Poco::Path::home() + ".leviathan/friends.json"));
        std::ifstream ifs(cfg_data.friends_json);
        if (ifs.is_open()) {
                namespace P = leviathan::platform;
                Poco::JSON::Array::Ptr pc =
                        P::utils::parse_json<Poco::JSON::Array::Ptr>(ifs);
                for (unsigned int i = 0; i < pc->size(); ++i) {
                        try {
                        Poco::JSON::Object::Ptr p = pc->getObject(i);
                        std::string n = p->getValue<std::string>("name");
                        std::string k = p->getValue<std::string>("public_key");
                        platform::utils::trim(k);
                        m_friends[n] = k;
                        } catch (Poco::Exception& e) {
                                METAX_WARNING(e.displayText());
                        }
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_device_json_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        Poco::Path p(Poco::Path::expand(
                                c->getString("device_json")));
        if (p.isAbsolute()) {
                cfg_data.device_json = p.toString();
        } else {
                cfg_data.device_json =
                        m_base_path + p.toString();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_peers_default_rating(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        uint16_t r = (unsigned short)c->getUInt("router.peers_default_rating",
                        cfg_data.peers_default_rating);
        if (100 >= r) {
                cfg_data.peers_default_rating = r;
        } else {
                METAX_WARNING("Invalid peers default rating : "
                                + platform::utils::to_string(r));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING("Invalid peers default rating : " + e.displayText());
}

void leviathan::metax::configuration_manager::
get_max_hops(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        uint16_t mh = (unsigned short)c->getUInt("router.max_hops",
                        cfg_data.max_hops);
        if (1 <= mh) {
                cfg_data.max_hops = mh;
        } else {
                METAX_WARNING("Invalid max hop count : "
                                + platform::utils::to_string(mh));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING("Invalid max hop count : " + e.displayText());
}

void leviathan::metax::configuration_manager::
get_peer_response_wait_time(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        uint16_t rt = (unsigned short)c->getUInt(
                        "router.peer_response_wait_time",
                        cfg_data.peer_response_wait_time);
        if (1 <= rt) {
                cfg_data.peer_response_wait_time = rt;
        } else {
                METAX_WARNING("Invalid peer response wait time : "
                                + platform::utils::to_string(rt));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING("Invalid peer response wait time : " + e.displayText());
}

void leviathan::metax::configuration_manager::
get_metax_info_path(const Poco::Util::XMLConfiguration* c)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        try {
                Poco::Path p(Poco::Path::expand(
                                        c->getString("metax_user_info")));
                if (p.isAbsolute()) {
                        cfg_data.metax_user_info_path = p.toString();
                } else {
                        cfg_data.metax_user_info_path =
                                m_base_path + p.toString();
                }
        } catch (Poco::RuntimeException& e) {
                METAX_WARNING(e.displayText() +
                        ". Defaulting to " + cfg_data.metax_user_info_path);
        }
        try {
                Poco::Path p(Poco::Path::expand(
                                        c->getString("metax_device_info")));
                if (p.isAbsolute()) {
                        cfg_data.metax_device_info_path = p.toString();
                } else {
                        cfg_data.metax_device_info_path =
                                m_base_path + p.toString();
                }
        } catch (Poco::RuntimeException& e) {
                METAX_WARNING(e.displayText() +
                        ". Defaulting to " + cfg_data.metax_device_info_path);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::configuration_manager::
get_config_version(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        cfg_data.version = c->getUInt("version");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() + ". Defaulting to "
                        + platform::utils::to_string(cfg_data.version));
}

void leviathan::metax::configuration_manager::
get_peer_verification_mode(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != c);
        std::string vm = c->getString("peer_verification_mode");
        platform::utils::trim(vm);
        if ("strict" == vm || "relaxed" == vm) {
                cfg_data.peer_verification_mode = vm;
        } else {
                cfg_data.peer_verification_mode = "none";
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::RuntimeException& e) {
        METAX_WARNING(e.displayText() + ". Defaulting to none");
        cfg_data.peer_verification_mode = "none";
}

void leviathan::metax::configuration_manager::
get_peers_ratings(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        Poco::Path p(Poco::Path::expand(c->getString("router.peers_rating")));
        cfg_data.peers_rating = p.isAbsolute() ? p.toString()
                : m_base_path + p.toString();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
get_user_json_info_path(const Poco::Util::XMLConfiguration* c)
try {
        METAX_TRACE(__FUNCTION__);
        Poco::Path p(Poco::Path::expand(
                                c->getString("user_json_info")));
        if (p.isAbsolute()) {
                cfg_data.user_json_info = p.toString();
        } else {
                cfg_data.user_json_info =
                        m_base_path + p.toString();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (Poco::Exception& e) {
        METAX_WARNING(e.displayText());
}

void leviathan::metax::configuration_manager::
expand_file_channels_path(Poco::Util::XMLConfiguration* c)
{
        Poco::Util::XMLConfiguration::Keys keys;
        c->keys("logging.channels", keys);
        for (auto const& k : keys) {
                std::string ch = "logging.channels." + k;
                if (c->has(ch + ".class") &&
                                "FileChannel" ==  c->getString(ch + ".class")) {
                        std::string f = Poco::Path::expand(
                                c->getString(ch + ".path", "leviathan.log"));
                        if (! Poco::Path(f).isAbsolute()
                                                     && ! m_base_path.empty()) {
                                f = m_base_path + f;
                        }
                        c->setString(ch + ".path", f);
                }
        }
}

void leviathan::metax::configuration_manager::
configure_logger(Poco::Util::XMLConfiguration* c)
try {
        expand_file_channels_path(c);
        Poco::Util::LoggingConfigurator log_cfg;
        log_cfg.configure(c);
        // Check logger configuration.
        // Note: The check is done only for configuration manager.
        // TODO - do the same check for all loggers.
        int l = m_logger.getLevel();
        m_logger.setLevel(Poco::Message::PRIO_INFORMATION);
        METAX_INFO("Logger configured.");
        m_logger.setLevel(l);
} catch (const Poco::Exception& e) {
        std::cerr << "Invalid configuration: " + e.displayText() << std::endl;
        std::terminate();
} catch (...) {
        std::cerr << "Invalid configuration." << std::endl;
        std::terminate();
}

void leviathan::metax::configuration_manager::
parse_config(const std::string& cfg_path)
{
        cfg_data.cfg_path = cfg_path;
        try {
                Poco::AutoPtr<Poco::Util::XMLConfiguration> c(
                                new Poco::Util::XMLConfiguration(cfg_path));
                configure_logger(c);
                cfg_data.port = c->getInt("metax_port", 7070);
                cfg_data.use_ssl = c->getBool("peer_use_ssl", true);
                get_device_key_path(c);
                get_user_key_path(c);
                get_storage_config(c);
                get_data_copy_count(c);
                get_storage_class(c);
                get_sync_class(c);
                get_enable_livestream_hosting(c);
                get_metadata_dump_timer(c);
                get_router_save_queue_size(c);
                get_router_get_queue_size(c);
                get_peer_info(c.get());
                get_trusted_peers_json_path(c);
                get_friends_json_path(c);
                get_device_json_path(c);
                get_user_json_info_path(c);
                get_peers_ratings(c);
                get_peers_default_rating(c);
                get_peer_response_wait_time(c);
                get_max_hops(c);
                get_metax_info_path(c);
                get_config_version(c);
                get_peer_verification_mode(c);
        } catch (...) {
                METAX_WARNING("Failed to parse config");
        }
}

void leviathan::metax::configuration_manager::
set_storage_configuration(storage_cfg_package& out)
{
        try {
                std::ifstream ifs(cfg_data.storage_path + "/storage.json");
                if (ifs.is_open()) {
                        using P = Poco::JSON::Object::Ptr;
                        P obj = platform::utils::parse_json<P>(ifs);
                        poco_assert(nullptr != obj);
                        if (obj->has("cache_size")) {
                                out.cache_size = obj->getValue<Poco::UInt64>("cache_size");
                        }
                        if (obj->has("storage_size")) {
                                out.storage_size = obj->getValue<Poco::UInt64>("storage_size");
                        }
                }
        } catch (const Poco::Exception& e) {
                METAX_WARNING(e.displayText());
        }
}

void leviathan::metax::configuration_manager::
configure_root_logger()
{
        //Poco::AutoPtr<Poco::ColorConsoleChannel> c(
        //                new Poco::ColorConsoleChannel);
        //Poco::AutoPtr<Poco::PatternFormatter> p(new Poco::PatternFormatter);
        //p->setProperty("pattern", "%p %Y-%m-%d %L%H:%M:%S:%i %s: %t");
        //Poco::AutoPtr<Poco::FormattingChannel> fc(
        //                new Poco::FormattingChannel(p, c));

        Poco::Logger& root = Poco::Logger::root();
        //root.setChannel(fc);
        root.setLevel(0);
}

leviathan::metax::configuration_manager::
configuration_manager(const std::string& cfg_path,
                const std::string& base_path)
        : platform::csp::task("configuration_manager",
                Poco::Logger::get("metax.configuration_manager"))
        , key_manager_rx(this)
        , storage_rx(this)
        , storage_writer_rx(this)
        , cache_rx(this)
        , wallet_rx(this)
        , link_rx(this)
        , router_rx(this)
        , kernel_rx(this)
        , backup_rx(this)
        , key_manager_tx(this)
        , storage_tx(this)
        , storage_writer_tx(this)
        , cache_tx(this)
        , wallet_tx(this)
        , link_tx(this)
        , router_tx(this)
        , kernel_tx(this)
        , backup_tx(this)
        , user_manager_tx(this)
        , cfg_data()
        , m_base_path(base_path)
        , m_trusted_peers()
        , m_friends()
        , m_md5_engine()
{
        configure_root_logger();
        parse_config(cfg_path);
}

leviathan::metax::configuration_manager::
~configuration_manager()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

