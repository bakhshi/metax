
/**
 * @file src/metax/key_manager.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::key_manager
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "key_manager.hpp"
#include "ssl_password_handler.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/RSAKey.h>
#include <Poco/Crypto/CryptoStream.h>
#include <Poco/FileStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/Util/Application.h>
#include <Poco/JSON/Parser.h>

// Headers from standard libraries
#include <string>
#include <sstream>
#include <fstream>

std::string leviathan::metax::key_manager::
encrypt_with_public_key(const std::string& p, const std::string& str)
{
        std::istringstream is(p);
        namespace C = Poco::Crypto;
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        C::RSAKey k = C::RSAKey(&is);
        C::Cipher::Ptr c = factory.createCipher(k);
        return c->encryptString(str, C::Cipher::ENC_BASE64);
}

std::string leviathan::metax::key_manager::
encrypt_data(Poco::Crypto::Cipher::Ptr aes_cipher)
{
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::encrypt == in.cmd ||
                                metax::stream_encrypt == in.cmd);
        poco_assert(nullptr != aes_cipher);
        std::string data(in.message.get(), in.size);
        return aes_cipher->encryptString(data);
}

void leviathan::metax::key_manager::
encrypt_data_with_new_aes_key()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::encrypt == in.cmd);
        poco_assert(in.aes_key.empty() && in.aes_iv.empty());
        Poco::Crypto::CipherKey aes_key(m_enc_type);
        namespace C = Poco::Crypto;
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        C::Cipher::Ptr aes_cipher = factory.createCipher(aes_key);
        std::string enc_data = encrypt_data(aes_cipher);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.set_payload(enc_data);
        out.aes_iv = platform::utils::base64_encode(aes_key.getIV());
        out.aes_key = platform::utils::base64_encode(aes_key.getKey());
        out.cmd = metax::encrypted;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_encrypt_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::encrypt == in.cmd);
        if (in.aes_key.empty() && in.aes_iv.empty()) {
                return encrypt_data_with_new_aes_key();
        }
        // Encrypt data with the given aes key.
        Poco::Crypto::CipherKey aes_key =
                get_aes_key(in.aes_key, in.aes_iv);
        poco_assert(in.size != 0);
        namespace C = Poco::Crypto;
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        C::Cipher::Ptr aes_cipher = factory.createCipher(aes_key);
        std::string enc_data = encrypt_data(aes_cipher);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.set_payload(enc_data);
        out.cmd = metax::encrypted;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_gen_key_for_stream()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::gen_key_for_stream == in.cmd);
        poco_assert(in.aes_key.empty() && in.aes_iv.empty());
        namespace C = Poco::Crypto;
        C::CipherKey aes_key(m_enc_type);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        C::Cipher::Ptr aes_cipher = factory.createCipher(aes_key);
        poco_assert(nullptr != aes_cipher);
        m_stream_ciphers[in.uuid] = aes_cipher;
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.aes_iv = platform::utils::base64_encode(aes_key.getIV());
        out.aes_key = platform::utils::base64_encode(aes_key.getKey());
        out.cmd = metax::encrypted;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_generate_aes_key()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::gen_aes_key == in.cmd);
        namespace C = Poco::Crypto;
        C::CipherKey aes_key(m_enc_type);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.aes_iv = platform::utils::base64_encode(aes_key.getIV());
        out.aes_key = platform::utils::base64_encode(aes_key.getKey());
        out.cmd = metax::get_aes_key;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
remove_tmp_files()
{
        METAX_TRACE(__FUNCTION__);
        Poco::File pub(m_user_key_path.path() + "/public.tmp");
        Poco::File priv(m_user_key_path.path() + "/private.tmp");
        Poco::File user_json_key(m_user_json_key_path + ".tmp");
        if (pub.exists()) {
                pub.remove();
        }
        if (priv.exists()) {
                priv.remove();
        }
        if (user_json_key.exists()) {
                user_json_key.remove();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax::key_manager::
get_user_json_uuid()
{
        METAX_TRACE(__FUNCTION__);
        try {
                if (Poco::File(m_user_json_key_path).exists()) {
                        using U = platform::utils;
                        using O = Poco::JSON::Object::Ptr;
                        std::ifstream ifs(m_user_json_key_path);
                        O o = U::parse_json<O>(ifs);
                        return o->optValue<std::string>("uuid", "");
                }
        } catch (const Poco::Exception& e) {
                METAX_ERROR(e.displayText());
        } catch (...) {
                METAX_ERROR("Failed to parse message");
        }
        return "";
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string
leviathan::metax::key_manager::
get_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        std::string s = m_user_key_path.path();
        std::string pb = platform::utils::read_file_content(s + "/public.pem");
        std::string pr = platform::utils::read_file_content(s + "/private.pem");
        Poco::JSON::Object::Ptr ujk = decrypt_user_json_info();
        Poco::JSON::Object::Ptr mi = new Poco::JSON::Object;
        if (pb.empty() || pr.empty() || nullptr == ujk) {
                throw Poco::Exception("Invalid user keys");
        }
        mi->set("user_public_key", pb);
        mi->set("user_private_key", pr);
        mi->set("user_json_key", ujk);
        std::ostringstream ostr;
        mi->stringify(ostr);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return ostr.str();
}

void leviathan::metax::key_manager::
handle_kernel_get_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::get_user_keys == in.cmd);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        try {
                out.set_payload(get_user_keys());
                out.cmd = metax::get_user_keys_response;
        } catch (const Poco::Exception& e) {
                out.set_payload("Failed to get user keys: " + e.displayText());
                out.cmd = metax::get_user_keys_failed;
        } catch (...) {
                out.set_payload("Failed to get user keys");
                out.cmd = metax::get_user_keys_failed;
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_regenerate_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::regenerate_user_keys == in.cmd);
        poco_assert(nullptr != in.message.get());
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        try {
                Poco::File pub(m_user_key_path.path() + "/public.tmp");
                Poco::File priv(m_user_key_path.path() + "/private.tmp");
                Poco::File user_json_key(m_user_json_key_path + ".tmp");
                create_user_key("public.tmp", "private.tmp", true);
                Poco::JSON::Object::Ptr obj = generate_new_user_json_key();
                obj->set("uuid", in.message.get());
                encrypt_user_json_info_with_user_key(obj,
                                m_user_json_key_path + ".tmp");
                pub.moveTo(m_user_key_path.path() + "/public.pem");
                priv.moveTo(m_user_key_path.path() + "/private.pem");
                user_json_key.moveTo(m_user_json_key_path);
                out.aes_key = obj->getValue<std::string>("aes_key");
                out.aes_iv = obj->getValue<std::string>("aes_iv");
                out.cmd = metax::regenerate_user_keys_succeeded;
        } catch (const Poco::Exception& e) {
                remove_tmp_files();
                out.set_payload(e.message());
                out.cmd = metax::failed;
        } catch (...) {
                remove_tmp_files();
                out.cmd = metax::failed;
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_prepare_cipher_for_stream()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::prepare_cipher_for_stream == in.cmd);
        poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
        namespace C = Poco::Crypto;
        Poco::Crypto::CipherKey aes_key =
                get_aes_key(in.aes_key, in.aes_iv);
        C::CipherFactory& factory = C::CipherFactory::defaultFactory();
        C::Cipher::Ptr aes_cipher = factory.createCipher(aes_key);
        poco_assert(nullptr != aes_cipher);
        m_stream_ciphers[in.uuid] = aes_cipher;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_stream_encrypt()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::stream_encrypt == in.cmd);
        poco_assert(in.size != 0);
        auto ci = m_stream_ciphers.find(in.uuid);
        poco_assert(m_stream_ciphers.end() != ci);
        std::string enc_data = encrypt_data(ci->second);
        kernel_key_manager_package& out = *kernel_tx;
        out.orig_size = in.size;
        out.request_id = in.request_id;
        out.set_payload(enc_data);
        out.cmd = metax::encrypted;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
send_stream_decrypt_fail_to_kernel(const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *kernel_rx;
        kernel_key_manager_package& out = *kernel_tx;
        out.cmd = metax::stream_decrypt_fail;
        out.set_payload(err);
        out.uuid = in.uuid;
        out.request_id = in.request_id;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
send_key_init_fail_to_kernel()
{
        METAX_TRACE(__FUNCTION__);
        kernel_key_manager_package& out = *kernel_tx;
        out.cmd = metax::key_init_fail;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
decrypt_stream()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::stream_decrypt == in.cmd);
        auto ci = m_stream_ciphers.find(in.uuid);
        if (m_stream_ciphers.end() == ci) {
                throw Poco::Exception("No cipher to decrypt");
        }
        std::string dec_data = decrypt_data(ci->second);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        out.set_payload(dec_data);
        out.cmd = metax::livestream_decrypted;
        out.uuid = in.uuid;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_stream_decrypt()
{
        METAX_TRACE(__FUNCTION__);
        try {
                decrypt_stream();
        } catch (const Poco::Exception& e) {
                send_stream_decrypt_fail_to_kernel("Failed to decrypt stream");
                METAX_ERROR("Failed to decrypt: " + e.displayText());
        } catch (...) {
                send_stream_decrypt_fail_to_kernel("Failed to decrypt data");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

std::string leviathan::metax::key_manager::
decrypt_data(Poco::Crypto::Cipher::Ptr aes_cipher)
{
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::decrypt == in.cmd || metax::stream_decrypt == in.cmd);
        poco_assert(nullptr != aes_cipher);
        std::string data(in.message.get(), in.size);
        return aes_cipher->decryptString(data);
}

std::string leviathan::metax::key_manager::
decrypt_portion_data(Poco::Crypto::Cipher::Ptr aes_cipher)
{
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::decrypt_with_portions == in.cmd);
        poco_assert(nullptr != aes_cipher);
        const int psize = 30016;
        std::string dec_data;
        int i;
        const char* msg = in.message.get();
        for ( i=0 ; i < in.size/psize ; i++) {
                const char* t = msg + i*psize;
                std::string data(t,psize);
                dec_data.append(aes_cipher->decryptString(data));
        }
        if ( i*psize < in.size ) {
                const char* t = msg + i*psize;
                std::string data(t,in.size - i*psize);
                dec_data.append(aes_cipher->decryptString(data));
        }
        return dec_data;
}

Poco::Crypto::CipherKey
leviathan::metax::key_manager::
get_aes_key(const std::string& aes_key, const std::string& aes_iv)
{
        namespace C = Poco::Crypto;
        std::string key = platform::utils::base64_decode(aes_key);
        std::string iv = platform::utils::base64_decode(aes_iv);
        C::CipherKey::ByteVec v_iv(iv.begin(), iv.end());
        C::CipherKey::ByteVec key_vec(key.begin(), key.end());
        C::CipherKey skey(m_enc_type, key_vec, v_iv);
        return skey;
}

void leviathan::metax::key_manager::
handle_kernel_decrypt_key()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        /// TODO - do proper error handling.
        poco_assert(nullptr != m_user_cipher);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::decrypt_key == in.cmd);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        try {
                poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
                std::string k = m_user_cipher->decryptString(in.aes_key,
                                Poco::Crypto::Cipher::ENC_BASE64);
                std::string i = m_user_cipher->decryptString(in.aes_iv,
                                Poco::Crypto::Cipher::ENC_BASE64);
                out.aes_key = platform::utils::base64_encode(k);
                out.aes_iv = platform::utils::base64_encode(i);
                out.cmd = metax::decrypted;
        } catch (const Poco::Exception& e) {
                out.cmd = metax::decryption_fail;
                std::string s = "Failed to decrypt key";
                out.set_payload(s + '.');
                METAX_INFO(s + ":" + e.displayText());
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_decrypt_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        /// TODO - do proper error handling.
        poco_assert(nullptr != m_user_cipher);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::decrypt == in.cmd);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        try {
                poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
                Poco::Crypto::CipherKey skey =
                        get_aes_key(in.aes_key, in.aes_iv);
                namespace C = Poco::Crypto;
                C::CipherFactory& factory =
                        C::CipherFactory::defaultFactory();
                C::Cipher::Ptr aes_cipher = factory.createCipher(skey);
                out.set_payload(decrypt_data(aes_cipher));
                out.cmd = metax::decrypted;
        } catch (const Poco::Exception& e) {
                out.cmd = metax::decryption_fail;
                std::string s = "Failed to decrypt data";
                out.set_payload(s + '.');
                METAX_INFO(s + ":" + e.displayText());
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_decrypt_with_portions_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        /// TODO - do proper error handling.
        poco_assert(nullptr != m_user_cipher);
        const kernel_key_manager_package& in = *kernel_rx;
        poco_assert(metax::decrypt_with_portions == in.cmd);
        kernel_key_manager_package& out = *kernel_tx;
        out.request_id = in.request_id;
        try {
                poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
                Poco::Crypto::CipherKey skey =
                        get_aes_key(in.aes_key, in.aes_iv);
                namespace C = Poco::Crypto;
                C::CipherFactory& factory =
                        C::CipherFactory::defaultFactory();
                C::Cipher::Ptr aes_cipher = factory.createCipher(skey);
                out.set_payload(decrypt_portion_data(aes_cipher));
                out.cmd = metax::decrypted;
        } catch (const Poco::Exception& e) {
                out.cmd = metax::decryption_fail;
                std::string s = "Failed to decrypt data";
                out.set_payload(s + '.');
                METAX_INFO(s + ":" + e.displayText());
        }
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_check_id_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        std::ifstream ifs(m_user_key_path.path() + "/public.pem");
        std::string c( (std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
        platform::utils::trim(c);
        kernel_key_manager_package& out = *kernel_tx;
        out = in;
        out.request_id = in.request_id;
        poco_assert(nullptr != in.message.get());
        out.cmd =
                (c == in.message.get()) ? metax::id_found : metax::id_not_found;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_kernel_reencrypt_key_request()
{
        METAX_TRACE(__FUNCTION__);
        kernel_key_manager_package& out = *kernel_tx;
        const kernel_key_manager_package& in = *kernel_rx;
        try {
                METAX_TRACE(__FUNCTION__);
                poco_assert(kernel_rx.has_data());
                poco_assert(metax::reencrypt_key == in.cmd);
                poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
                std::string pb = in.message.get();
                out.aes_iv = encrypt_with_public_key(pb,
                                platform::utils::base64_decode(in.aes_iv));
                out.aes_key = encrypt_with_public_key(pb,
                                platform::utils::base64_decode(in.aes_key));
                out.cmd = metax::reencrypted_key;
        } catch (const Poco::FileException& e) {
                METAX_ERROR("Failed reencrypt: " + e.displayText());
                out.cmd = metax::reencryption_fail;
                out.set_payload(std::string("Reencryption failed"));
        }
        out.request_id = in.request_id;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
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

void leviathan::metax::key_manager::
process_kernel_input()
{
        poco_assert(kernel_rx.has_data());
        const kernel_key_manager_package& in = *kernel_rx;
        switch (in.cmd) {
                case metax::encrypt: {
                        handle_kernel_encrypt_request();
                        break;
                } case metax::decrypt: {
                        handle_kernel_decrypt_request();
                        break;
                } case metax::decrypt_with_portions: {
                        handle_kernel_decrypt_with_portions_request();
                        break;
                } case metax::check_id: {
                        handle_kernel_check_id_request();
                        break;
                } case metax::reencrypt_key: {
                        handle_kernel_reencrypt_key_request();
                        break;
                } case metax::gen_key_for_stream: {
                        handle_kernel_gen_key_for_stream();
                        break;
                } case metax::prepare_cipher_for_stream: {
                        handle_kernel_prepare_cipher_for_stream();
                        break;
                } case metax::stream_encrypt: {
                        handle_kernel_stream_encrypt();
                        break;
                } case metax::stream_decrypt: {
                        handle_kernel_stream_decrypt();
                        break;
                } case metax::decrypt_key: {
                        handle_kernel_decrypt_key();
                        break;
                } case metax::gen_aes_key: {
                        handle_kernel_generate_aes_key();
                        break;
                } case metax::regenerate_user_keys: {
                        handle_kernel_regenerate_user_keys();
                        break;
                } case metax::get_user_keys: {
                        handle_kernel_get_user_keys();
                        break;
                } default: {
                        METAX_WARNING("Not handled command !!");
                }
        }
}

void leviathan::metax::key_manager::
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
                METAX_ERROR("Unable to initialize keys: " + e.displayText());
                abort();
        } catch (const std::exception& e) {
                std::string msg = "Unable to initialize keys: ";
                METAX_ERROR(msg + e.what());
                abort();
        } catch (...) {
                METAX_ERROR("Unhandled exception");
                abort();
        }
        link_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::key_manager::
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
                METAX_ERROR("Unable to initialize keys: " + e.displayText());
                send_key_init_fail_to_kernel();
        } catch (const std::exception& e) {
                std::string msg = "Unable to initialize keys: ";
                METAX_ERROR(msg + e.what());
                send_key_init_fail_to_kernel();
        } catch (...) {
                METAX_ERROR("Unhandled exception");
                send_key_init_fail_to_kernel();
        }
        config_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
process_config_input()
{
        poco_assert(config_rx.has_data());
        const key_manager_cfg_package& in = *config_rx;
        switch (in.cmd) {
                case metax::key_path: {
                        handle_config_key_path();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
}

void leviathan::metax::key_manager::
process_link_input()
{
        poco_assert(link_rx.has_data());
        const link_key_manager_package& in = *link_rx;
        switch (in.cmd) {
                case metax::request_keys_response: {
                        handle_link_request_keys_respons(in);
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
}

void leviathan::metax::key_manager::
write_in_file(const std::string& k, const std::string& file_name)
{
        METAX_TRACE(__FUNCTION__);
        Poco::File tmp(file_name + ".tmp");
        std::ofstream file_stream;
        file_stream.open(tmp.path());
        if (k.empty() || file_stream.fail()) {
                throw Poco::Exception("Unable to overwrite file: " + file_name);
        }
        file_stream.write(k.c_str(), k.size());
        file_stream.close();
        tmp.moveTo(file_name);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::key_manager::
overwrite_user_keys(const link_key_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        write_in_file(in.public_key, m_user_key_path.path() + "/public.pem");
        write_in_file(in.private_key, m_user_key_path.path() + "/private.pem");
        write_in_file(in.user_json_info, m_user_json_key_path);
        Poco::Crypto::CipherFactory &factory = 
                Poco::Crypto::CipherFactory::defaultFactory();
        m_user_cipher = factory.createCipher(Poco::Crypto::RSAKey(
                                m_user_key_path.path() + "/public.pem",
                                m_user_key_path.path() + "/private.pem",
                                ssl_password_handler::METAX_SSL_PASSPHRASE));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
handle_link_request_keys_respons(const link_key_manager_package& in)
try {
        poco_assert(in.cmd == metax::request_keys_response);
        overwrite_user_keys(in);
        Poco::JSON::Object::Ptr o = decrypt_user_json_info();
        poco_assert(nullptr != o);
        kernel_key_manager_package& out = (*kernel_tx); 
        out.cmd = metax::request_keys_response;
        out.aes_key = o->getValue<std::string>("aes_key");
        out.aes_iv = o->getValue<std::string>("aes_iv");
        out.request_id = in.request_id;
        kernel_tx.commit();
} catch (const Poco::Exception& e) {
        kernel_key_manager_package& out = (*kernel_tx); 
        out.cmd = metax::request_keys_fail;
        out.request_id = in.request_id;
        out = e.message();
        kernel_tx.commit();
}

void leviathan::metax::key_manager::
handle_config_key_path()
{
        poco_assert(config_rx.has_data());
        poco_assert((*config_rx).cmd == metax::key_path);
        poco_assert(nullptr != (*config_rx).message.get());
        std::istringstream istr((*config_rx).message.get());
        std::string device_key;
        istr >> device_key;
        std::string user_key;
        istr >> user_key;
        m_user_key_path = user_key;
        istr >> m_user_json_key_path;
        create_device_key(device_key);
        create_user_key();
        create_user_json_key();
        (*config_tx).cmd = metax::initialized_keys;
        config_tx.commit();
}

X509* leviathan::metax::key_manager::
generate_x509(EVP_PKEY* pkey)
{
        X509 * x509 = X509_new();
        if(! x509) {
                throw Poco::Exception("Unable to create X509 structure.");
        }
        // Set the serial number
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        // This certificate is valid from now until exactly ten year from now.
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 630720000L);
        // Set the public key for our certificate
        X509_set_pubkey(x509, pkey);
        // Copy the subject name to the issuer name
        X509_NAME * n = X509_get_subject_name(x509);
        // Set the country code and common name
        X509_NAME_add_entry_by_txt(n, "C",  MBSTRING_ASC,
                        (const unsigned char*)"AM", -1, -1, 0);
        X509_NAME_add_entry_by_txt(n, "O",  MBSTRING_ASC,
                        (const unsigned char*)"Leviathan CJSC", -1, -1, 0);
        X509_NAME_add_entry_by_txt(n, "CN", MBSTRING_ASC,
                        (const unsigned char*)"ggg.leviathan.am", -1, -1, 0);
        // Set the issuer name
        X509_set_issuer_name(x509, n);
        // Actually sign the certificate with our key
        if (! X509_sign(x509, pkey, EVP_sha1())) {
                X509_free(x509);
                throw Poco::Exception("Error signing certificate.");
        }
        return x509;
}

void leviathan::metax::key_manager::
generate_certificate(const Poco::File& device_key_path)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Crypto::EVPPKey pek(
                        device_key_path.path() + "/public.pem",
                        device_key_path.path() + "/private.pem",
                        ssl_password_handler::METAX_SSL_PASSPHRASE);
        EVP_PKEY* pkey = pek;
        X509* x509 = generate_x509(pkey);
        std::string p = device_key_path.path() + "/cert.pem";
        FILE * x509_file = fopen(p.c_str(), "wb");
        if (! x509_file) {
                X509_free(x509);
                throw Poco::Exception("Unable to open cert.pem for writing");
        }
        bool ret = PEM_write_X509(x509_file, x509);
        fclose(x509_file);
        if (! ret) {
                X509_free(x509);
                throw Poco::Exception("Unable to save certificate in file.");
        }
        X509_free(x509);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::key_manager::
create_device_key(Poco::File device_key_path)
{
        device_key_path.createDirectories();
        namespace C = Poco::Crypto;
        if (! Poco::File(device_key_path.path() + "/private.pem").exists()) {
                METAX_INFO("Device key not found, generating ...");
                C::RSAKey rsa(C::RSAKey::KL_4096, C::RSAKey::EXP_SMALL);
                rsa.save(device_key_path.path() + "/public.pem",
                        device_key_path.path() + "/private.pem",
                        ssl_password_handler::METAX_SSL_PASSPHRASE);
                generate_certificate(device_key_path);
                METAX_INFO("Device key generation done");
        }
        if (! Poco::File(device_key_path.path() + "/cert.pem").exists()) {
                generate_certificate(device_key_path);
        }
        C::CipherFactory &factory = C::CipherFactory::defaultFactory();
        m_device_cipher = factory.createCipher(Poco::Crypto::RSAKey(
                                device_key_path.path() + "/public.pem",
                                device_key_path.path() + "/private.pem",
                                ssl_password_handler::METAX_SSL_PASSPHRASE));
}

void leviathan::metax::key_manager::
create_user_key(const std::string& pub, const std::string& priv, bool check)
{
        m_user_key_path.createDirectories();
        namespace C = Poco::Crypto;
        if (check ||
              ! Poco::File(m_user_key_path.path() + "/" + priv).exists()) {
                METAX_INFO("User key not found, generating ...");
                C::RSAKey rsa(C::RSAKey::KL_4096, C::RSAKey::EXP_SMALL);
                rsa.save(m_user_key_path.path() + "/" + pub,
                        m_user_key_path.path() + "/" + priv,
                        ssl_password_handler::METAX_SSL_PASSPHRASE);
                METAX_INFO("User key generation done");
        }
        C::CipherFactory &factory = C::CipherFactory::defaultFactory();
        m_user_cipher = factory.createCipher(Poco::Crypto::RSAKey(
                                m_user_key_path.path() + "/" + pub,
                                m_user_key_path.path() + "/" + priv,
                                ssl_password_handler::METAX_SSL_PASSPHRASE));
}

void leviathan::metax::key_manager::
create_user_json_key()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr obj;
        if (Poco::File(m_user_json_key_path).exists()) {
                obj = parse_user_info_as_json();
                poco_assert(nullptr != obj);
        } else {
                obj = generate_new_user_json_key();
                poco_assert(nullptr != obj);
                Poco::UUIDGenerator& g =
                        Poco::UUIDGenerator::defaultGenerator();
                obj->set("uuid", g.createRandom().toString());
                (*config_tx).is_generated = true;
                encrypt_user_json_info_with_user_key(obj, m_user_json_key_path);
        }
        (*config_tx).user_json_info_ptr = obj;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::JSON::Object::Ptr leviathan::metax::key_manager::
decrypt_user_json_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_cipher);
        std::stringstream decrypt_sink;
        Poco::FileInputStream ifs(m_user_json_key_path);
        poco_assert(nullptr != m_user_cipher);
        Poco::Crypto::CryptoInputStream decryptor(ifs,
                        m_user_cipher->createDecryptor());
        Poco::StreamCopier::copyStream(decryptor, decrypt_sink);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return platform::utils::
                parse_json<Poco::JSON::Object::Ptr>(decrypt_sink);
}

Poco::JSON::Object::Ptr leviathan::metax::key_manager::
parse_user_info_as_json()
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr obj;
        try {
                Poco::FileInputStream ifs(m_user_json_key_path);
                obj = platform::utils::parse_json<Poco::JSON::Object::Ptr>(ifs);
                encrypt_user_json_info_with_user_key(obj, m_user_json_key_path);
        } catch (...) {
                METAX_WARNING("Unable to parse user json info as json. "
                                "Trying to decrypt."); 
                obj = decrypt_user_json_info();

        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return obj;
}

Poco::JSON::Object::Ptr leviathan::metax::key_manager::
generate_new_user_json_key()
{
        Poco::Crypto::CipherKey aes_key(m_enc_type);
        std::string iv = platform::utils::base64_encode(aes_key.getIV());
        std::string key = platform::utils::base64_encode(aes_key.getKey());
        Poco::JSON::Object::Ptr obj = new Poco::JSON::Object;
        obj->set("aes_iv", iv);
        obj->set("aes_key", key);
        return obj;
}

void leviathan::metax::key_manager::
encrypt_user_json_info_with_user_key(Poco::JSON::Object::Ptr obj,
                const std::string& key_path)
{
        std::string p = Poco::Path(key_path).parent().toString();
        Poco::File d(p);
        if ("" != p && ! d.exists()) {
                Poco::File(p).createDirectories();
        }
        Poco::FileOutputStream sink(key_path, std::ios::out | std::ios::binary
                                                            | std::ios::trunc);
        Poco::Crypto::CryptoOutputStream encryptor(sink,
                        m_user_cipher->createEncryptor());
        std::stringstream ss;
        obj->stringify(ss);
        Poco::StreamCopier::copyStream(ss, encryptor);
        encryptor.close();
        sink.close();
}

void leviathan::metax::key_manager::
runTask()
try {
        while (true) {
                if (!wait()) {
                        break;
                }
                handle_kernel_input();
                handle_config_input();
                handle_link_input();
        }
        finish();
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
}

leviathan::metax::key_manager::
key_manager()
        : platform::csp::task("key_manager", Poco::Logger::get("metax.key_manager"))
        , kernel_rx(this)
        , config_rx(this)
        , link_rx(this)
        , kernel_tx(this)
        , config_tx(this)
        , link_tx(this)
        , m_user_key_path("")
        , m_user_json_key_path()
        , m_enc_type("aes256")
        , m_device_cipher(nullptr)
        , m_user_cipher(nullptr)
        , m_stream_ciphers()
{
}

leviathan::metax::key_manager::
~key_manager()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

