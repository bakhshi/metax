/**
 * @file src/metax/protocols.hpp
 *
 * @brief Definition of protocols used between metax layers and peers
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_PROTOCOLS_HPP
#define LEVIATHAN_METAX_PROTOCOLS_HPP

// Headers from this project

// Headers from other projects
#include <platform/default_package.hpp>

// Headers from third party libraries
#include <Poco/UUID.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/SharedPtr.h>
#include <Poco/ByteOrder.h>
#include <Poco/TemporaryFile.h>

// Headers from standard libraries
#include <set>
#include <cstring>
#include <vector>

// Defines package types
#define LINK_PACKAGE 0x01
#define ROUTER_PACKAGE 0x02
#define KERNEL_PACKAGE 0x04

/**
 * Defines domains for request ids
 * Domain is defined based on most significient 4 bits
 * Up to 16 domains can be defined
 */

// NOTE 0xFFFFFFFF value is reserverd for representing unitialized/invalid
// request ids
#define KERNEL_REQUEST_LOWER_BOUND 0x00000000
#define KERNEL_REQUEST_UPPER_BOUND 0x0FFFFFFF
#define ROUTER_REQUEST_LOWER_BOUND 0x10000000
#define ROUTER_REQUEST_UPPER_BOUND 0x1FFFFFFF
#define LINK_REQUEST_LOWER_BOUND   0x20000000
#define LINK_REQUEST_UPPER_BOUND   0x2FFFFFFF
#define IMS_REQUEST_LOWER_BOUND    0x30000000
#define IMS_REQUEST_UPPER_BOUND    0x3FFFFFFF
#define REQUEST_MAX_VALUE          0xFFFFFFFF

#define UUID_SIZE    16
#define DATA_VERSION_START 1
#define DEFAULT_EXPIRE_DATE 0
#define MASTER_VERSION 1

// Forward declarations
namespace leviathan {
        namespace metax {
                typedef char _8_byte_ [8];
                typedef char _16_byte_ [16];
                typedef char _64_byte_ [64];
                typedef char _256_byte_ [256];
                typedef char _1024_byte_ [1024];
                typedef char _4096_byte_ [4096];
                typedef Poco::UUID UUID;
                typedef uint16_t ID16;
                typedef uint32_t ID32;
                typedef Poco::UInt64 ID64;

                struct link_cfg_package;
                struct storage_cfg_package;
                struct link_peer_package;
                struct router_link_package;
                struct kernel_router_package;
                struct kernel_storage_package;
                struct kernel_key_manager_package;
                struct kernel_cfg_package;
                struct ims_kernel_package;
                struct router_cfg_package;
                struct router_data;
                struct link_data;
                struct key_manager_cfg_package;
                struct kernel_backup_package;
                struct kernel_user_manager_package;
                struct backup_cfg_package;
                struct backup_user_manager_package;
                struct key_transfer_package;
                struct link_key_manager_package;

                /**
                 * Definition of commands used as internally as well as externally.
                 */
                enum command
                {
                        // Commands used between Link <-> CFG , Link <-> Link Peer
                        connect, disconnect, connected, disconnected, ack, nack, bind, shutdown,
                        // Link <-> Link Peer
                        data, // used to notify data transfer
                        /**
                         * Router <-> Link 
                         * Send - send message to peers
                         * Broadcast - send message to all link_peers. peers listed in peers
                         * member are ingnored. 
                         * Broadcast_except - send message to all link_peers except the ones
                         * listed in the peers vector.
                         * Received - message is received from link_peer listed in peers member
                         */
                        send, broadcast, broadcast_except, received_data,
                        // IMS -> Kernel, Kernel -> IMS
                        create, id, prepare_streaming, clean_stream_request,
                        /// Router <-> Router, Router <-> Kernel, IMS <-> Kernel
                        get, save, auth, del, deleted, get_data, save_confirm,
                        share_confirm, save_offer, update, updated, share, send_to_peer,
                        save_fail, get_fail, share_fail, send_to_fail, reencryption_fail,
                        decryption_fail, router_fail, deliver_fail, update_fail, failed,

                        /// Router <-> Router, Router <-> Kernel
                        auth_data, save_data, cancel, finalize,  find_peer,
                        peer_found, send_to_peer_confirm,
                        /// IMS <-> Kernel, Kernel <-> IMS, Kernel <-> Config
                        remove_peer, remove_peer_confirm, remove_peer_failed,
                        add_peer, add_peer_confirm, get_peer_list, get_peer_list_response,
                        //-------------------
                        clear_request,
                        //------------------
                        // Router <-> Kernel
                        // response to internal request(router to kernel). Based on this
                        // response tender got from remote peer will be
                        // distributed/broadcasted.
                        refuse,
                        // Router<-> Router, Router<->Kernel
                        no_permissions,
                        // Kernel -> IMS
                        no_resources,
                        // Kernel -> DB
                        look_up, 

                        // CFG -> Key Manager
                        key_path,
                        storage_cfg, add_in_cache,
                        write_storage_cfg,
                        delete_from_cache,
                        // Kernel <-> Key Manager
                        encrypt, encrypted, decrypt, decrypted, check_id,
                        id_found, id_not_found, reencrypt_key, reencrypted_key,
                        // Kernel <-> IMS, Kernel <-> Storage
                        move_cache_to_storage, move_cache_to_storage_completed,

                        /// Router <-> Router, Router <-> Kernel, IMS <-> Kernel
                        on_update_register, on_update_registered, on_update_register_fail,

                        /// Kernel <-> Config
                        get_public_key, get_public_key_response, update_listeners_path,

                        // Kernel <-> Config
                        kernel_cfg,

                        /// Router <-> Router, Router <-> Kernel, IMS <-> Kernel
                        save_stream, save_stream_offer, livestream_content,
                        livestream_cancel,

                        // Kernel <-> Kernel
                        get_livestream_content,

                        // Kernel <-> Key Manager
                        gen_key_for_stream, stream_encrypt, stream_decrypt, prepare_cipher_for_stream,
                        cancel_livestream_get,

                        // Kernel <-> Config, Key Manager <-> Config
                        user_json,
                        initialized_keys,
                        accept_share, share_accepted, accept_share_fail,
                        decrypt_key,
                        save_stream_fail,
                        get_livestream_fail,
                        livestream_decrypted,
                        stream_auth,
                        stream_decrypt_fail,
                        delete_fail,
                        notify_update,
                        device_id,
                        get_online_peers,
                        get_online_peers_response,
                        copy,
                        copy_succeeded,
                        copy_failed,
                        router_config,

                        // IMS <-> Kernel <-> Config
                        get_metax_info, get_metax_info_response,

                        // Kernel -> Backup
                        journalize,

                        // Config -> Backup
                        backup_cfg,

                        // link_peer <-> link <-> router <-> kernel <-> backup
                        send_journal_info,
                        get_journal_info,
                        send_uuids,
                        get_uuid,
                        del_uuids,

                        // IMS <-> kernel <-> router <-> link
                        reconnect_to_peers,
                        start_storage_sync,

                        // kernel <-> router
                        notify_delete,

                        // Router <-> Router, Router <-> Kernel, IMS <-> Kernel
                        on_update_unregister, on_update_unregistered,
                        on_update_unregister_fail,
                        sync_finished,

                        // IMS <-> Kernel <-> Config
                        set_metax_info, set_metax_info_ok, set_metax_info_fail,

                        // Router <-> Router, Router <-> Kernel, IMS <-> Kernel
                        recording_start, recording_stop, recording_fail,
                        recording_started, recording_stopped,

                        // IMS <-> Kernel <-> Config
                        get_user_keys, get_user_keys_response,

                        // Kernel <-> Key Manager
                        gen_aes_key, get_aes_key,

                        // IMS <-> Kernel
                        dump_user_info, dump_user_info_succeeded,

                        // Router <-> Link
                        sync_started,

                        // IMS <-> Kernel <-> Config <-> link
                        start_pairing, get_generated_code,
                        cancel_pairing,
                        get_pairing_peers, get_pairing_peers_response,
                        request_keys, request_keys_response,

                        regenerate_user_keys,
                        regenerate_user_keys_succeeded,
                        regenerate_user_keys_fail,

                        start_pairing_fail, request_keys_fail,
                        add_peer_failed,

                        /// IMS <-> Kernel, Kernel <-> IMS, Kernel <-> Config
                        add_friend, add_friend_confirm, add_friend_failed,
                        get_friend_list, get_friend_list_response,
                        get_friend, get_friend_found, get_friend_not_found, // Get public key /confirm
                        no_space,

                        /// Link <-> Config
                        verify_peer, peer_verified, peer_not_verified,
                        /// Key manager <-> Kernel, Kernel <-> IMS
                        get_user_keys_failed,

                        /// Key manager <-> Kernel, Kernel <-> User manager
                        key_init_fail, decrypt_with_portions,

                        max_command
                };
        }
}

/**
 * @brief Definition of the protocol between kernel and configuration manager
 *
 */
struct leviathan::metax::kernel_cfg_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        /// currently it is public key of a user
        platform::default_package user_id;
        unsigned int storage_class;
        unsigned int sync_class;
        bool enable_livestream_hosting;
        uint16_t data_copy_count;
        uint32_t router_save_queue_size;
        uint32_t router_get_queue_size;

        kernel_cfg_package& operator= (const default_package& p)
        {
                this->default_package::operator= (p);
                return *this;
        }

        kernel_cfg_package()
                : request_id(0)
                , cmd(max_command)
                , user_id()
        {}

        ~kernel_cfg_package()
        {}
};

/**
 * @brief Definition of the protocol between router and configuration manager
 *
 */
struct leviathan::metax::router_cfg_package
        : leviathan::platform::default_package
{
        command cmd;
        Poco::UInt16 max_hops;
        Poco::UInt16 peer_response_wait_time;

        router_cfg_package()
                : cmd(max_command)
        {}

        ~router_cfg_package()
        {}
};

/**
 * @brief Definition of the protocol between link and configuration manager
 *
 */
struct leviathan::metax::link_cfg_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        ID32 peer_id;
        platform::default_package device_id;

        link_cfg_package()
                : request_id(0)
                , cmd(max_command)
        {}

        ~link_cfg_package()
        {}
};

/**
 * @brief Definition of the protocol between link and configuration manager
 *
 */
struct leviathan::metax::key_manager_cfg_package
        : leviathan::platform::default_package
{
        std::string path;
        command cmd;
        Poco::JSON::Object::Ptr user_json_info_ptr;
        bool is_generated;

        key_manager_cfg_package()
                : path()
                , cmd(max_command)
                , user_json_info_ptr(nullptr)
                , is_generated(false)
        {}

        ~key_manager_cfg_package()
        {}
};

/**
 * @brief Definition of the protocol between storage and configuration manager
 *
 */
struct leviathan::metax::storage_cfg_package
        : leviathan::platform::default_package
{
        command cmd;
        double cache_size_limit_in_gb;
        double size_limit_in_gb;
        double cache_age;
        Poco::UInt64 cache_size;
        Poco::UInt64 storage_size;


        storage_cfg_package()
                : cmd(max_command)
                , cache_size_limit_in_gb(0)
                , size_limit_in_gb(0)
                , cache_age(0)
                , cache_size(0)
                , storage_size(0)
        {}

        ~storage_cfg_package()
        {}
};

/**
 * @brief Definition of the protocol between link and link_peer
 */
struct leviathan::metax::link_peer_package
        : leviathan::platform::default_package
{

        ID32 request_id;
        command cmd;

        link_peer_package& operator= (const link_peer_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                this->default_package::operator= (p);
                return *this;
        }
        link_peer_package& operator= (const default_package& p)
        {
                this->default_package::operator= (p);
                return *this;
        }

        link_peer_package()
                : default_package()
                , request_id(0)
                , cmd(max_command)
        {}

        ~link_peer_package()
        {
        }
};


/**
 * @brief Definition of the protocol between router and link
 */
struct leviathan::metax::router_link_package
        : leviathan::platform::default_package
{
        ID32 request_id;

        /**
         * Send - send message to peers
         * Broadcast - send message to all link_peers. peers listed in peers
         * member are ingnored. 
         * Broadcast_except - send message to oll link_peers except the ones
         * listed in the peers vector.
         * Received - message is received from link_peer listed in peers member
         */
        command cmd;

        /**
         * List of peers to send/received from message.
         */
        std::set<ID32> peers;

        /**
         * Rating of the command. Used by link layer to decide broadcast the
         * command to a peer or not depending on the peers rating.
         */
        unsigned int cmd_rating;

        bool should_block_by_sync;

        router_link_package& operator= (const router_link_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                peers = p.peers;
                cmd_rating = p.cmd_rating;
                should_block_by_sync = p.should_block_by_sync;
                this->default_package::operator=(p);
                return *this;
        }

        router_link_package& operator= (const default_package& p)
        {
                this->default_package::operator=(p);
                return *this;
        }

        router_link_package()
                : request_id(0)
                , cmd(max_command)
                , peers()
                , cmd_rating(0)
                , should_block_by_sync(false)
        {}

        ~router_link_package()
        {
        }

};

/**
 * @brief Definition of the protocol between link layers of two peers
 */
struct leviathan::metax::link_data
        : leviathan::platform::default_package
{

        command cmd;
        std::string public_key;
        ID32 load_info;

        void serialize(default_package& pkg)
        {
                pkg.size = (uint32_t)(header_size() + sizeof(size) + size);
                pkg.message = new char[pkg.size];
                char* p = pkg.message.get();
                unsigned long int offset = 0;
                uint32_t n_p = Poco::ByteOrder::toNetwork(version);
                std::memcpy(p + offset, (char*)&n_p, sizeof(version));
                offset += sizeof(version);
                command n_cmd = (command)Poco::ByteOrder::toNetwork(cmd);
                std::memcpy(p + offset, (char*)&n_cmd, sizeof(cmd));
                offset += sizeof(cmd);
                uint32_t s = (uint32_t)public_key.size();
                uint32_t n_s = Poco::ByteOrder::toNetwork(s);
                std::memcpy(p + offset, (char*)&n_s, sizeof(n_s));
                offset += sizeof(n_s);
                std::memcpy(p + offset, public_key.c_str(), s);
                offset += s;
                ID32 n_li = (command)Poco::ByteOrder::toNetwork(load_info);
                std::memcpy(p + offset, (char*)&n_li, sizeof(n_li));
                offset += sizeof(n_li);
                uint32_t sf = size | (((uint32_t)flags << 24) & 0xFF000000);
                uint32_t n_sf = Poco::ByteOrder::toNetwork(sf);
                std::memcpy(p + offset, (char*)&n_sf, sizeof(n_sf));
                offset += sizeof(size);
                if (0 != size) {
                        std::memcpy(p + offset, message, size);
                }
        }

        uint32_t extract_header(const default_package& pkg)
        {
                unsigned long int offset = 0;
                const char* src = pkg.message.get();
                uint32_t n_p;
                std::memcpy((char*)&n_p, src + offset, sizeof(version));
                version = Poco::ByteOrder::fromNetwork(n_p);
                offset += sizeof(version);
                command n_cmd;
                std::memcpy((char*)&n_cmd, src + offset, sizeof(cmd));
                cmd = (command)Poco::ByteOrder::fromNetwork(n_cmd);
                offset += sizeof(cmd);
                uint32_t s_p;
                std::memcpy((char*)&s_p, src + offset, sizeof(uint32_t));
                uint32_t s = Poco::ByteOrder::fromNetwork(s_p);
                offset += sizeof(uint32_t);
                public_key.assign(src + offset, s);
                offset += s;
                ID32 n_li;
                std::memcpy((char*)&n_li, src + offset, sizeof(n_li));
                load_info = (command)Poco::ByteOrder::fromNetwork(n_li);
                offset += sizeof(load_info);
                return (uint32_t) offset;
        }

        uint32_t header_size() const
        {
                return (uint32_t)(sizeof(cmd)
                                + public_key.size()
                                + sizeof(uint32_t)
                                + sizeof(version)
                                + sizeof(ID32));
        }

        link_data()
                : cmd(max_command)
        {}

        ~link_data()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and router
 */
struct leviathan::metax::kernel_router_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        UUID uuid;
        uint16_t max_response_cnt;
        uint16_t winners_count;
        uint32_t data_version;
        Poco::UInt64 last_updated;
        Poco::UInt64 expire_date;

        kernel_router_package& operator= (const kernel_router_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                max_response_cnt = p.max_response_cnt;
                winners_count = p.winners_count;
                data_version = p.data_version;
                last_updated = p.last_updated;
                expire_date = p.expire_date;
                default_package::operator=(p);
                return *this;
        }

        kernel_router_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        kernel_router_package()
                : request_id(0)
                , cmd(max_command)
                , uuid()
                , max_response_cnt(1)
                , winners_count(0)
                , data_version(0)
                , last_updated(0)
                , expire_date(0)
        {}

        kernel_router_package(const kernel_router_package& d)
                : platform::default_package()
                , request_id(d.request_id)
                , cmd(d.cmd)
                , uuid(d.uuid)
                , max_response_cnt(d.max_response_cnt)
                , winners_count(d.winners_count)
                , data_version(0)
                , last_updated(0)
                , expire_date(0)
        {
                size = d.size;
                message = d.message;
        }

        ~kernel_router_package()
        {
        }
};

/**
 * @brief Definition of the protocol between routers of two peers
 */
struct leviathan::metax::router_data
        : leviathan::platform::default_package
{

        UUID tender_id;
        command cmd;
        UUID uuid;
        // Defines id of the current hop for the tender
        ID16 hop_id;
        uint32_t data_version;
        Poco::UInt64 last_updated;
        Poco::UInt64 expire_date;
        std::vector<ID32> winners_id;

        void serialize(default_package& pkg)
        {
                pkg.size = (uint32_t)(header_size() + sizeof(size) + size);
                pkg.message = new char[pkg.size];
                char* p = pkg.message.get();
                unsigned long int offset = 0;
                uint32_t n_p = Poco::ByteOrder::toNetwork(version);
                std::memcpy(p + offset, (char*)&n_p, sizeof(version));
                offset += sizeof(version);
                tender_id.copyTo(p + offset);
                offset += UUID_SIZE;
                command n_cmd = (command)Poco::ByteOrder::toNetwork(cmd);
                std::memcpy(p + offset, (char*)&n_cmd, sizeof(cmd));
                offset += sizeof(cmd);
                uuid.copyTo(p + offset);
                offset += UUID_SIZE;
                ID16 n_hop_id = Poco::ByteOrder::toNetwork(hop_id);
                std::memcpy(p + offset, (char*)&n_hop_id, sizeof(hop_id));
                offset += sizeof(hop_id);

                // serialize data version
                uint32_t n_dversion = Poco::ByteOrder::toNetwork(data_version);
                std::memcpy(p + offset, (char*)&n_dversion,
                                sizeof(n_dversion));
                offset += sizeof(n_dversion);

                // serialize last updated date 
                Poco::UInt64 n_updated =
                        Poco::ByteOrder::toNetwork(last_updated);
                std::memcpy(p + offset, (char*)&n_updated, sizeof(n_updated));
                offset += sizeof(n_updated);

                // serialize expiration date
                Poco::UInt64 n_expire = Poco::ByteOrder::toNetwork(expire_date);
                std::memcpy(p + offset, (char*)&n_expire, sizeof(n_expire));
                offset += sizeof(n_expire);

                Poco::UInt64 ws = winners_id.size();
                Poco::UInt64 n_ws = Poco::ByteOrder::toNetwork(ws);
                std::memcpy(p + offset, (char*)&n_ws, sizeof(n_ws));
                offset += sizeof(n_ws);
                for (size_t i = 0; i < winners_id.size(); ++i) {
                        ID32 n_wid = Poco::ByteOrder::toNetwork(winners_id[i]);
                        std::memcpy(p + offset, (char*)&n_wid, sizeof(n_wid));
                        offset += sizeof(n_wid);
                }
                // TODO serialize also flag member of default_package data
                // structure.
                uint32_t sf = size | (((uint32_t)flags << 24) & 0xFF000000);
                uint32_t n_sf = Poco::ByteOrder::toNetwork(sf);
                std::memcpy(p + offset, (char*)&n_sf, sizeof(n_sf));
                offset += sizeof(size);
                if (0 != size) {
                        std::memcpy(p + offset, message, size);
                }
        }
        
        uint32_t extract_header(const default_package& pkg)
        {
                unsigned long int offset = 0;
                const char* src = pkg.message.get();
                uint32_t n_p;
                std::memcpy((char*)&n_p, src + offset, sizeof(version));
                version = Poco::ByteOrder::fromNetwork(n_p);
                offset += sizeof(version);
                tender_id.copyFrom(src + offset);
                offset += UUID_SIZE;
                command n_cmd;
                std::memcpy((char*)&n_cmd, src + offset, sizeof(cmd));
                cmd = (command)Poco::ByteOrder::fromNetwork(n_cmd);
                offset += sizeof(cmd);
                uuid.copyFrom(src + offset);
                offset += UUID_SIZE;
                ID16 n_hop_id;
                std::memcpy((char*)&n_hop_id, src + offset, sizeof(hop_id));
                hop_id = Poco::ByteOrder::fromNetwork(n_hop_id);
                offset += sizeof(hop_id);

                uint32_t n_dversion;
                std::memcpy((char*)&n_dversion, src + offset,
                                sizeof(data_version));
                data_version = Poco::ByteOrder::fromNetwork(n_dversion);
                offset += sizeof(data_version);

                Poco::UInt64 n_updated;
                std::memcpy((char*)&n_updated, src + offset,
                                sizeof(last_updated));
                last_updated = Poco::ByteOrder::fromNetwork(n_updated);
                offset += sizeof(last_updated);

                Poco::UInt64 n_expire;
                std::memcpy((char*)&n_expire, src + offset,
                                sizeof(expire_date));
                expire_date = Poco::ByteOrder::fromNetwork(n_expire);
                offset += sizeof(expire_date);

                Poco::UInt64 n_ws;
                std::memcpy((char*)&n_ws, src + offset, sizeof(n_ws));
                Poco::UInt64 ws = Poco::ByteOrder::fromNetwork(n_ws);
                offset += sizeof(n_ws);
                for (Poco::UInt64 i = 0; i < ws; ++i) {
                        ID32 n_wid;
                        std::memcpy((char*)&n_wid, src + offset, sizeof(n_wid));
                        winners_id.push_back(
                                        Poco::ByteOrder::fromNetwork(n_wid));
                        offset += sizeof(n_wid);
                }

                return (uint32_t) offset;
        }

        void reset()
        {
                tender_id = UUID::null();
                cmd = metax::max_command;
                uuid = UUID::null();
                default_package::reset();
                winners_id.clear();
        }

        uint32_t header_size() const 
        {
                return (uint32_t)(UUID_SIZE + sizeof(cmd) +
                                  UUID_SIZE + sizeof(hop_id) +
                                  sizeof(data_version) +
                                  sizeof(last_updated) +
                                  sizeof(expire_date) +
                                  sizeof(version) +
                                  winners_id.size() * sizeof(ID32) +
                                  sizeof(Poco::UInt64));
        }

        // Updates hop_id in the serialized router data with the specified one
        static void update_hop_id(char* c, ID16 h)
        {
                ID32 b = (uint32_t)(UUID_SIZE + sizeof(version) +
                                    sizeof(cmd) + UUID_SIZE);
                ID16 n_h = Poco::ByteOrder::toNetwork(h);
                std::memcpy(c + b, (const char*)&n_h, sizeof(h));
        }

        router_data& operator= (const router_data& p)
        {
                cmd = p.cmd;
                tender_id = p.tender_id;
                uuid = p.uuid;
                hop_id = p.hop_id;
                data_version = p.data_version;
                last_updated = p.last_updated;
                expire_date = p.expire_date;
                winners_id = p.winners_id;
                default_package::operator =(p);
                return *this;
        }

        router_data& operator= (const default_package& p)
        {
                default_package::operator =(p);
                return *this;
        }

        router_data()
                : tender_id(UUID::null())
                , cmd(max_command)
                , uuid()
                , hop_id ()
                , data_version(0)
                , last_updated(0)
                , expire_date(0)
                , winners_id()
        {}

        router_data(const router_data& d)
                : platform::default_package()
                , tender_id(d.tender_id)
                , cmd(d.cmd)
                , uuid()
                , data_version(0)
                , last_updated(0)
                , expire_date(0)
                , winners_id()
        {
                uuid = d.uuid;
                size = d.size;
                message = d.message;
        }

        ~router_data()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and key_manager
 */
struct leviathan::metax::kernel_key_manager_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        UUID uuid;
        std::string aes_key;
        std::string aes_iv;
        std::string path;
        uint32_t orig_size;

        kernel_key_manager_package& operator= (const kernel_key_manager_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                aes_key = p.aes_key;
                aes_iv = p.aes_iv;
                path = p.path;
                orig_size = p.orig_size;
                default_package::operator=(p);
                return *this;
        }

        kernel_key_manager_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        kernel_key_manager_package()
                : request_id()
                , cmd(max_command)
                , uuid(UUID::null())
                , aes_key()
                , path()
                , orig_size(0)
        {}

        ~kernel_key_manager_package()
        {
        }

};

/**
 * @brief Definition of the protocol between ims and kernel
 */
struct leviathan::metax::ims_kernel_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        UUID uuid;
        UUID rec_uuid;
        // Use when data is provided by stream.
        std::string file_path;
        Poco::Int64 expire_date;

        // for sending response as tme file
        Poco::SharedPtr<Poco::TemporaryFile> response_file_path;

        // Used on delete requests.
        bool keep_chunks;

        std::string content_type;
        bool enc;
        /// currently it is public key of a user
        platform::default_package user_id;
        // when the requested data in get request is a media file and metax is
        // going to stream the content, this member represents the count of the
        // chunks of the streamed content
        Poco::UInt64 chunk_count;
        std::pair<int64_t, int64_t> get_range;
        Poco::UInt64 content_length;
        bool cache;

        // Used to send aes key as response when share request is received.
        std::string aes_key;
        std::string aes_iv;

        enum save_option : int8_t {
                DEFAULT = -1,
                NO_LOCAL = 0,
                LOCAL = 1
        };
        save_option local;

        ims_kernel_package& operator= (const ims_kernel_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                rec_uuid = p.rec_uuid;
                file_path = p.file_path;
                expire_date = p.expire_date;
                response_file_path = p.response_file_path;
                keep_chunks = p.keep_chunks;
                content_type = p.content_type;
                enc = p.enc;
                local = p.local;
                user_id = p.user_id;
                default_package::operator=(p);
                chunk_count = p.chunk_count;
                get_range = p.get_range;
                content_length = p.content_length;
                cache = p.cache;
                aes_key = p.aes_key;
                aes_iv = p.aes_iv;
                return *this;
        }

        ims_kernel_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        ims_kernel_package()
                : request_id(0)
                , cmd(max_command)
                , uuid()
                , rec_uuid()
                , file_path()
                , expire_date(0)
                , response_file_path()
                , keep_chunks(false)
                , content_type()
                , enc(true)
                , user_id()
                , chunk_count(0)
                , get_range(std::make_pair(-1, -1))
                , content_length(0)
                , cache(true)
                , aes_key()
                , aes_iv()
                , local(DEFAULT)
        {}

        ~ims_kernel_package()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and DB
 */
struct leviathan::metax::kernel_storage_package
        : leviathan::platform::default_package
{
        // Type of the data requested from storage - data, metadata, etc
        //enum type { data, metadata, max_type};

        ID32 request_id;
        command cmd;
        UUID uuid;
        bool should_send_response;
        bool check_cache;


        kernel_storage_package& operator= (const kernel_storage_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                check_cache = p.check_cache;
                should_send_response = p.should_send_response;
                default_package::operator=(p);
                return *this;
        }

        kernel_storage_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        kernel_storage_package()
                : request_id()
                , cmd(max_command)
                , uuid()
                , should_send_response(true)
                , check_cache(true)
        {}

        ~kernel_storage_package()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and backup
 */
struct leviathan::metax::backup_cfg_package
        : leviathan::platform::default_package
{
        command cmd;
        std::string metadata_path;
        Poco::JSON::Object::Ptr user_json_info_ptr;
        bool is_generated;
        uint16_t periodic_interval;

        backup_cfg_package& operator= (const backup_cfg_package& p)
        {
                cmd = p.cmd;
                metadata_path = p.metadata_path;
                user_json_info_ptr = p.user_json_info_ptr;
                is_generated = p.is_generated;
                periodic_interval = p.periodic_interval;
                default_package::operator=(p);
                return *this;
        }

        backup_cfg_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        backup_cfg_package()
                : cmd(max_command)
                , metadata_path()
                , user_json_info_ptr(nullptr)
                , is_generated(false)
                , periodic_interval(0)
        {}

        ~backup_cfg_package()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and backup
 */
struct leviathan::metax::kernel_backup_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        UUID uuid;
        Poco::Timestamp time;
        /*
         * '0' - created
         * '1' - updated
         * '2' - deleted
         */
        char operation;

        union {
                uint32_t data_version;
                uint32_t sync_class;
        };
        Poco::UInt64 last_updated;
        Poco::UInt64 expire_date;
        bool is_deleted;
        Poco::JSON::Object::Ptr sync_uuids;
        std::string key;
        std::string iv;


        kernel_backup_package& operator= (const kernel_backup_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                time = p.time;
                operation = p.operation;
                data_version = p.data_version;
                last_updated = p.last_updated;
                expire_date = p.expire_date;
                is_deleted = p.is_deleted;
                sync_uuids = p.sync_uuids;
                key = p.key;
                iv = p.iv;
                default_package::operator=(p);
                return *this;
        }

        kernel_backup_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        kernel_backup_package()
                : request_id()
                , cmd(max_command)
                , uuid()
                , time()
                , operation()
                , data_version(0)
                , last_updated(0)
                , expire_date(0)
                , is_deleted(false)
                , sync_uuids(nullptr)
                , key()
                , iv()
        {}

        ~kernel_backup_package()
        {
        }
};

/**
 * @brief Definition of the protocol between kernel and user manager
 */
struct leviathan::metax::kernel_user_manager_package
        : leviathan::platform::default_package
{
        ID32 request_id;
        command cmd;
        Poco::UUID uuid;
        std::string iv;
        std::string key;
        bool owned;

        union {
                Poco::UInt64 copy_count;
                Poco::UInt32 data_version;
        };

        Poco::UInt64 last_updated;

        kernel_user_manager_package& operator=(
                                        const kernel_user_manager_package& p)
        {
                request_id = p.request_id;
                cmd = p.cmd;
                uuid = p.uuid;
                iv = p.iv;
                key = p.key;
                owned = p.owned;
                copy_count = p.copy_count;
                last_updated = p.last_updated;
                default_package::operator=(p);
                return *this;
        }

        kernel_user_manager_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        kernel_user_manager_package()
                : request_id()
                , cmd(max_command)
                , uuid()
                , iv("")
                , key("")
                , owned(true)
                , copy_count(1)
                , last_updated(0)
        {}

        ~kernel_user_manager_package()
        {
        }

};

/**
 * @brief Definition of the protocol between kernel and user manager
 */
struct leviathan::metax::backup_user_manager_package
        : leviathan::platform::default_package
{
        command cmd;
        Poco::UUID uuid;
        Poco::UInt64 data_version;
        Poco::UInt64 last_updated;

        backup_user_manager_package& operator=(
                                        const backup_user_manager_package& p)
        {
                cmd = p.cmd;
                uuid = p.uuid;
                data_version = p.data_version;
                last_updated = p.last_updated;
                default_package::operator=(p);
                return *this;
        }

        backup_user_manager_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        backup_user_manager_package()
                : cmd(max_command)
                , uuid()
                , data_version(0)
                , last_updated(0)
        {}

        ~backup_user_manager_package()
        {
        }

};

/**
 * @brief Definition of the protocol between peers during key transfer
 */
struct leviathan::metax::key_transfer_package
        : leviathan::platform::default_package
{
        command cmd;
        std::string code;
        std::string public_key;
        std::string private_key;
        std::string user_json_info;

        void serialize(default_package& pkg)
        {
                pkg.size = (uint32_t)(header_size() + sizeof(size) + size);
                pkg.message = new char[pkg.size];
                char* p = pkg.message.get();
                unsigned long int offset = 0;

                uint32_t n_p = Poco::ByteOrder::toNetwork(version);
                std::memcpy(p + offset, (char*)&n_p, sizeof(version));
                offset += sizeof(version);

                command n_cmd = (command)Poco::ByteOrder::toNetwork(cmd);
                std::memcpy(p + offset, (char*)&n_cmd, sizeof(cmd));
                offset += sizeof(cmd);

                uint32_t cs = (uint32_t)code.size();
                uint32_t n_cs = Poco::ByteOrder::toNetwork(cs);
                std::memcpy(p + offset, (char*)&n_cs, sizeof(n_cs));
                offset += sizeof(n_cs);
                std::memcpy(p + offset, code.c_str(), cs);
                offset += cs;

                uint32_t pbks = (uint32_t)public_key.size();
                uint32_t n_pbks = Poco::ByteOrder::toNetwork(pbks);
                std::memcpy(p + offset, (char*)&n_pbks, sizeof(n_pbks));
                offset += sizeof(n_pbks);
                std::memcpy(p + offset, public_key.c_str(), pbks);
                offset += pbks;

                uint32_t prks = (uint32_t)private_key.size();
                uint32_t n_prks = Poco::ByteOrder::toNetwork(prks);
                std::memcpy(p + offset, (char*)&n_prks, sizeof(n_prks));
                offset += sizeof(n_prks);
                std::memcpy(p + offset, private_key.c_str(), prks);
                offset += prks;

                uint32_t ujis = (uint32_t)user_json_info.size();
                uint32_t n_ujis = Poco::ByteOrder::toNetwork(ujis);
                std::memcpy(p + offset, (char*)&n_ujis, sizeof(n_ujis));
                offset += sizeof(n_ujis);
                std::memcpy(p + offset, user_json_info.c_str(), ujis);
                offset += ujis;

                uint32_t sf = size | (((uint32_t)flags << 24) & 0xFF000000);
                uint32_t n_sf = Poco::ByteOrder::toNetwork(sf);
                std::memcpy(p + offset, (char*)&n_sf, sizeof(n_sf));
                offset += sizeof(n_sf);
                if (0 != size) {
                        std::memcpy(p + offset, message, size);
                }
        }

        uint32_t extract_header(const default_package& pkg)
        {
                unsigned long int offset = 0;
                const char* src = pkg.message.get();

                uint32_t n_p;
                std::memcpy((char*)&n_p, src + offset, sizeof(version));
                version = Poco::ByteOrder::fromNetwork(n_p);
                offset += sizeof(version);

                command n_cmd;
                std::memcpy((char*)&n_cmd, src + offset, sizeof(cmd));
                cmd = (command)Poco::ByteOrder::fromNetwork(n_cmd);
                offset += sizeof(cmd);

                uint32_t n_cs;
                std::memcpy((char*)&n_cs, src + offset, sizeof(uint32_t));
                uint32_t cs = Poco::ByteOrder::fromNetwork(n_cs);
                offset += sizeof(uint32_t);
                code.assign(src + offset, cs);
                offset += cs;

                uint32_t n_pbks;
                std::memcpy((char*)&n_pbks, src + offset, sizeof(uint32_t));
                uint32_t pbks = Poco::ByteOrder::fromNetwork(n_pbks);
                offset += sizeof(uint32_t);
                public_key.assign(src + offset, pbks);
                offset += pbks;

                uint32_t n_prks;
                std::memcpy((char*)&n_prks, src + offset, sizeof(uint32_t));
                uint32_t prks = Poco::ByteOrder::fromNetwork(n_prks);
                offset += sizeof(uint32_t);
                private_key.assign(src + offset, prks);
                offset += prks;

                uint32_t n_ujis;
                std::memcpy((char*)&n_ujis, src + offset, sizeof(uint32_t));
                uint32_t ujis = Poco::ByteOrder::fromNetwork(n_ujis);
                offset += sizeof(uint32_t);
                user_json_info.assign(src + offset, ujis);
                offset += ujis;

                return (uint32_t)offset;
        }

        uint32_t header_size() const
        {
                return (uint32_t)(sizeof(cmd)
                                + code.size()
                                + sizeof(uint32_t)
                                + public_key.size()
                                + sizeof(uint32_t)
                                + private_key.size()
                                + sizeof(uint32_t)
                                + user_json_info.size()
                                + sizeof(uint32_t)
                                + sizeof(version));
        }

        key_transfer_package& operator=(
                                        const key_transfer_package& p)
        {
                cmd = p.cmd;
                code = p.code;
                public_key = p.public_key;
                private_key = p.private_key;
                user_json_info = p.user_json_info;
                default_package::operator=(p);
                return *this;
        }

        key_transfer_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        key_transfer_package()
                : cmd(max_command)
                , code("")
                , public_key("")
                , private_key("")
                , user_json_info("")
        {}

        ~key_transfer_package()
        {
        }

};

/**
 * @brief Definition of the protocol between peers during key transfer
 */
struct leviathan::metax::link_key_manager_package
        : leviathan::metax::key_transfer_package
{
        ID32 request_id;

        link_key_manager_package& operator=(const key_transfer_package& p)
        {
                cmd = p.cmd;
                code = p.code;
                public_key = p.public_key;
                private_key = p.private_key;
                user_json_info = p.user_json_info;
                default_package::operator=(p);
                return *this;
        }

        link_key_manager_package& operator= (const default_package& p)
        {
                default_package::operator=(p);
                return *this;
        }

        link_key_manager_package()
                : request_id()
        {}

        ~link_key_manager_package()
        {
        }

};

#endif // LEVIATHAN_METAX_PROTOCOLS_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

