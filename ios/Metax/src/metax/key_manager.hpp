/**
 * @file src/metax/key_manager.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::key_manager
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_KEY_MANAGER_HPP
#define LEVIATHAN_METAX_KEY_MANAGER_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include "Poco/Crypto/Cipher.h"
#include <Poco/Crypto/CipherKey.h>
#include <Poco/File.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct key_manager;
        }
}

/**
 * @brief Does key management, generates RSA and AES keys, performs all
 * encryption/decryption tasks.
 * 
 */
struct leviathan::metax::key_manager:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        using INPUT = platform::csp::input<platform::default_package>;
        using KERNEL_RX = platform::csp::input<kernel_key_manager_package>;
        using OUTPUT = platform::csp::output<platform::default_package>;
        using KERNEL_TX = platform::csp::output<kernel_key_manager_package>;

        using CONFIG_RX = platform::csp::input<key_manager_cfg_package>;
        using CONFIG_TX = platform::csp::output<key_manager_cfg_package>;
        using LINK_RX = platform::csp::input<link_key_manager_package>;
        using LINK_TX = platform::csp::output<link_key_manager_package>;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

private:
        void handle_kernel_input();
        void process_kernel_input();
        void handle_config_input();
        void handle_link_input();
        void process_config_input();
        void process_link_input();
        void handle_config_key_path();
        void handle_kernel_encrypt_request();
        void handle_kernel_gen_key_for_stream();
        void handle_kernel_prepare_cipher_for_stream();
        void handle_kernel_stream_encrypt();
        void handle_kernel_stream_decrypt();
        void handle_kernel_decrypt_request();
        void handle_kernel_decrypt_with_portions_request();
        void handle_kernel_decrypt_key();
        void handle_kernel_check_id_request();
        void handle_kernel_reencrypt_key_request();
        void handle_link_request_keys_respons(
                        const link_key_manager_package& in);
        void write_in_file(const std::string& k, const std::string& file_name);
        Poco::JSON::Object::Ptr decrypt_user_json_info();
        void overwrite_user_keys(const link_key_manager_package& in);
        std::string get_user_keys();
        void handle_kernel_get_user_keys();

        /**
         * @brief Encrypts data in vec by specified publiy key
         *        and encodes with BASE 64
         */
        std::string encrypt_with_public_key(const std::string& p,
                        const std::string& vec);
        std::string encrypt_data(Poco::Crypto::Cipher::Ptr aes_cipher);
        std::string decrypt_data(Poco::Crypto::Cipher::Ptr aes_cipher);
        std::string decrypt_portion_data(Poco::Crypto::Cipher::Ptr aes_cipher);
        Poco::Crypto::CipherKey get_aes_key(const std::string&,
                        const std::string&);
        void encrypt_data_with_new_aes_key();
        X509* generate_x509(EVP_PKEY* pkey);
        void generate_certificate(const Poco::File& device_key_path);
        void create_device_key(Poco::File);
        void create_user_key(const std::string& pub = "public.pem",
                             const std::string& priv = "private.pem",
                             bool check = false);
        void create_user_json_key();
        Poco::JSON::Object::Ptr generate_new_user_json_key();
        void handle_kernel_regenerate_user_keys();
        void remove_tmp_files();
        std::string get_user_json_uuid();
        void encrypt_user_json_info_with_user_key(Poco::JSON::Object::Ptr obj,
                        const std::string& key_path);
        Poco::JSON::Object::Ptr parse_user_info_as_json();
        void send_stream_decrypt_fail_to_kernel(const std::string& err);
        void send_key_init_fail_to_kernel();
        void decrypt_stream();
        void handle_kernel_generate_aes_key();
public:
        KERNEL_RX kernel_rx;
        CONFIG_RX config_rx;
        LINK_RX link_rx;

        KERNEL_TX kernel_tx;
        CONFIG_TX config_tx;
        LINK_TX link_tx;

        /// @name Private data members
private:
        Poco::File m_user_key_path;
        std::string m_user_json_key_path;
        std::string m_enc_type;
        Poco::Crypto::Cipher::Ptr m_device_cipher;
        Poco::Crypto::Cipher::Ptr m_user_cipher;
        std::map<UUID, Poco::Crypto::Cipher::Ptr> m_stream_ciphers;

        /// @name Special member functions
public:
        /// Default constructor
        key_manager();

        /// Destructor
        virtual ~key_manager();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        key_manager(const key_manager&);

        /// This class is not assignable
        key_manager& operator=(const key_manager&);
        
        /// This class is not copy-constructible
        key_manager(const key_manager&&);

        /// This class is not assignable
        key_manager& operator=(const key_manager&&);

}; // class leviathan::metax::key_manager

#endif // LEVIATHAN_METAX_KEY_MANAGER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

