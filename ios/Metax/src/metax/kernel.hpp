/**
 * @file src/metax/kernel.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::kernel
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_KERNEL_HPP
#define LEVIATHAN_METAX_KERNEL_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/JSON/Object.h>
#include <Poco/ExpireCache.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Timer.h>
#include <Poco/StringTokenizer.h>

// Headers from standard libraries
#include <unordered_map>
#include <list>
#include <forward_list>
#include <memory>


// Forward declarations
namespace leviathan {
        namespace metax {
                struct kernel;
                struct storage;
        }
}

/**
 * @brief Implements the main logic of Metax. It receives (almost) all requests
 * from both own external input queue and from peers and decides to which task
 * it should be forwarded.
 *
 */
struct leviathan::metax::kernel:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        using INPUT = platform::csp::input<platform::default_package>;
        using ROUTER_RX = platform::csp::input<kernel_router_package>;
        using STORAGE_RX = platform::csp::input<kernel_storage_package>;
        using IMS_RX = platform::csp::input<ims_kernel_package>;
        using KEY_MANAGER_RX = platform::csp::input<kernel_key_manager_package>;
        using CONFIG_RX = platform::csp::input<kernel_cfg_package>;
        using BACKUP_RX = platform::csp::input<kernel_backup_package>;
        using USER_MANAGER_RX =
                platform::csp::input<kernel_user_manager_package>;

        using OUTPUT = platform::csp::output<platform::default_package>;
        using ROUTER_TX = platform::csp::output<kernel_router_package>;
        using STORAGE_TX = platform::csp::output<kernel_storage_package>;
        using IMS_TX = platform::csp::output<ims_kernel_package>;
        using KEY_MANAGER_TX =
                platform::csp::output<kernel_key_manager_package>;
        using CONFIG_TX = platform::csp::output<kernel_cfg_package>;
        using BACKUP_TX = platform::csp::output<kernel_backup_package>;
        using USER_MANAGER_TX =
                platform::csp::output<kernel_user_manager_package>;

        /// @name Private type definitions
private:
        struct request;

        struct chunk_streaming_info {
                bool requested;
                bool available;
                Poco::UInt64 start;
                Poco::UInt64 size;
                Poco::UUID uuid;
                Poco::SharedPtr<Poco::TemporaryFile> file_path;

                chunk_streaming_info()
                : requested(false)
                , available(false)
                , start(0)
                , size(0)
                , uuid(Poco::UUID::null())
                , file_path(new Poco::TemporaryFile(
                                        platform::utils::tmp_dir))
                {}
        };

        struct streaming_info {
                Poco::UInt64 curr;
                Poco::UInt64 last;
                Poco::SharedPtr<std::vector<chunk_streaming_info>> list;

                streaming_info()
                : curr(0)
                , last(0)
                , list(nullptr)
                {}

                ~streaming_info()
                {
                }
        };

        /*
         * @brief Stores information about recording when livestream also
         * should be recorded. Stores uuid of the recording, and whether stop
         * request is received for the recording. If stop request is received
         * then after appending the next chunk the recording will be removed
         * from the livestream recording's list and no new chunks will be
         * appended.
         */
        struct recording_info {
                Poco::UUID uuid;
                Poco::JSON::Object::Ptr master;
                Poco::JSON::Object::Ptr playlist;
                Poco::Timestamp start_time;

                recording_info()
                : uuid()
                , master()
                , playlist()
                , start_time()
                {}

                recording_info(const Poco::UUID& u,
                                Poco::JSON::Object::Ptr m, bool c = false)
                : uuid(u)
                , master(m)
                , playlist()
                , start_time()
                {}

                ~recording_info()
                {}
        };

        /*
         * @brief Keeps information for processing livestreams hosted on this
         * node. Keeps the list of requests (remote or local) listening to the
         * livestream and also stores the id of request keeping track of
         * livestream hosting
         */
        struct livestream_info {
                ID32 request_id;
                std::vector<ID32> listeners;
                std::vector<recording_info> recordings;
                std::string rec_chunk;
                uint32_t rec_chunk_size;

                livestream_info(ID32 rid)
                : request_id(rid)
                , listeners()
                , recordings()
                , rec_chunk()
                , rec_chunk_size(0)
                {}

                ~livestream_info()
                {
                }
        };

        struct request
        {
                enum state {lookup_in_db, requested, authentication,
                                receiving, sending, save_accept, max_state };
                enum type {t_ims, t_router, t_kernel, t_backup,
                                        t_user_manager, max_type };

                ID32 request_id;
                UUID uuid;
                state st;
                command cmd;
                // Used when deleting master
                bool keep_chunks;
                type typ;
                platform::default_package data;
                unsigned short fail_count;
                // Contains the content of master json file
                Poco::JSON::Object::Ptr master_json;
                ID32 parent_req_id;
                ID32 size;

                // Used when data is provided by stream
                std::string file_path;

                /**
                 * Used to send response with temporary file
                 * The last user will remove also the file from tmp directory
                 */
                Poco::SharedPtr<Poco::TemporaryFile> response_file_path;

                std::string content_type;

                // Specifies the confirmed response count for update request.
                unsigned short updated;

                // Specifies whether the data should be encrypted or not.
                // By default the data should be encrypted.
                bool enc;

                // AES key (and initialization vector) encryted with user
                // public key and base64 encoded.
                std::string aes_key;
                std::string aes_iv;

                // UUID-s of old pieces which should be deleted when update
                // request finishes.
                Poco::JSON::Object::Ptr old_master_json;

                streaming_info streaming_state;

                std::pair<int64_t, int64_t> get_range;
                bool cache;
                std::set<ID32> units;
                std::set<ID32> backup_units;
                std::set<Poco::UUID> saved_units;

                // Specifies whether the data should be saved in local storage
                // or not.
                bool local;

                uint16_t winners_count;

                UUID copy_uuid;

                // reused by livestream content to provide to peer hosting the
                // livestream the original size of video chunk before
                // encryption
                uint32_t data_version;

                Poco::UInt64 last_updated;

                Poco::UInt64 expire_date;

                // Removes the uuid from pieces.
                void remove_uuid_from_master_json(const UUID& u)
                {
                        poco_assert(nullptr != master_json);
                        Poco::JSON::Object::Ptr p =
                                master_json->getObject("pieces");
                        poco_assert(nullptr != p);
                        poco_assert(p->has(u.toString()));
                        p->remove(u.toString());
                }

                // Returns true if pieces of master json is empty.
                bool is_master_json_empty()
                {
                        poco_assert(nullptr != master_json);
                        Poco::JSON::Object::Ptr p =
                                        master_json->getObject("pieces");
                        poco_assert(nullptr != p);
                        return 0 == p->size();
                }

                request(ID32 id, const UUID& u, state s,  command c, type t)
                        : request_id(id)
                        , st(s)
                        , cmd(c)
                        , keep_chunks(false)
                        , typ(t)
                        , data()
                        , fail_count(0)
                        , parent_req_id(REQUEST_MAX_VALUE)
                        , size(0)
                        , file_path("")
                        , response_file_path()
                        , content_type("")
                        , updated(0)
                        , enc(true)
                        , aes_key("")
                        , aes_iv("")
                        , old_master_json(0)
                        , streaming_state()
                        , cache(true)
                        , local(true)
                        , winners_count(0)
                        , copy_uuid()
                        , data_version(DATA_VERSION_START)
                        , expire_date(0)

                {
                        Poco::Timestamp now;
                        last_updated = now.epochMicroseconds();
                        get_range.first = -1;
                        get_range.second = -1;
                        uuid = u;
                }

                request(ID32 id, const UUID& u, state s,  command c, type t,
                                uint32_t v, uint64_t lu, uint64_t ed)
                        : request_id(id)
                        , st(s)
                        , cmd(c)
                        , keep_chunks(false)
                        , typ(t)
                        , data()
                        , fail_count(0)
                        , parent_req_id(REQUEST_MAX_VALUE)
                        , size(0)
                        , file_path("")
                        , response_file_path()
                        , content_type("")
                        , updated(0)
                        , enc(true)
                        , aes_key("")
                        , aes_iv("")
                        , old_master_json(0)
                        , streaming_state()
                        , cache(true)
                        , local(true)
                        , winners_count(0)
                        , copy_uuid()
                        , data_version(v)
                        , last_updated(lu)
                        , expire_date(ed)

                {
                        get_range.first = -1;
                        get_range.second = -1;
                        uuid = u;
                }

                request()
                        : request_id()
                        , uuid()
                        , st(max_state)
                        , cmd(max_command)
                        , keep_chunks(false)
                        , typ(max_type)
                        , data()
                        , fail_count(0)
                        , parent_req_id(REQUEST_MAX_VALUE)
                        , size(0)
                        , file_path("")
                        , response_file_path()
                        , content_type("")
                        , updated(0)
                        , enc(true)
                        , aes_key("")
                        , aes_iv("")
                        , old_master_json(0)
                        , streaming_state()
                        , cache(true)
                        , local(true)
                        , winners_count(0)
                        , copy_uuid()
                        , data_version(DATA_VERSION_START)
                        , expire_date(0)
                {
                        Poco::Timestamp now;
                        last_updated = now.epochMicroseconds();
                        get_range.first = -1;
                        get_range.second = -1;
                }

                ~request()
                {
                }
        };

        struct save_unit
        {
                enum unit_type {master, piece};

                request* main_request;
                unsigned int start;
                unsigned int size;
                unsigned int bkp_count;
                unit_type type;
                UUID uuid;
                platform::default_package content;
                command cmd;
                unsigned int fail_count;
                uint16_t winners_count;
                bool backup;

                save_unit(request* r, unsigned int st, unsigned int sz,
                                unsigned int bc, unit_type t, const UUID u,
                                platform::default_package cnt, command c,
                                bool b)
                        : main_request(r)
                        , start(st)
                        , size(sz)
                        , bkp_count(bc)
                        , type(t)
                        , uuid(u)
                        , content(cnt)
                        , cmd(c)
                        , fail_count(0)
                        , winners_count(0)
                        , backup(b)
                {
                }
                save_unit() = default;

        };

        /// uuid -> request_id map for IMS -> kernel queries;
        typedef std::map<UUID, std::vector<ID32> > uuid_request_map;
        /// map of all requests
        typedef std::map<ID32, request> requests;
        /// map for kernel and ims request id correspondence,
        // <Kernel's generated id> -> <IMS generated id>
        typedef std::map<ID32, ID32> kernel_ims_req_ids;

        /// map for request id and backup ids.
        using backup_ids = std::map<ID32, std::vector<ID32> >;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask();

        /// @name Helper functions
private:
        void wait_for_input_channels();

        void send_response_to_ims_unhandled_command();

        void handle_router_start_storage_sync();

        void handle_storage_start_sync();

        /**
         * @brief - Send metax::lookup_in_db command to storage for uuid of
         * given request.
         *
         * @param r - the request object which uuid should be searched in
         * storage.
         *
         * @param check_cache - look also in cache or not
         */
        void post_storage_look_up_request(const request& r,
                                                bool check_cache = true);

        bool should_save_in_local(ims_kernel_package::save_option l) const;

        /**
         * @brief - Send metax::get command to storage for uuid of given
         * request.
         *
         * @param req - the request object which uuid should be get.
         */
        void post_storage_get_request(request& req);

        /**
         * @brief - Send metax::no_permissions command to ims for given request.
         *
         * @param req - the request object for which should be send
         * no permissions response to ims.
         */
        void post_ims_no_permissions(request& req);

        void authenticate_request(request& r);
        void add_update_listener_for_local_request(request& r);
        void add_update_listener_for_remote_request(request& r);

        /**
         * @brief - registers new listener on update event for the received
         * UIID.
         *
         * @detail - If the request received from the local IMS, then empty
         * string is stored as the peer ID, if the request is from the remote
         * peer, then user public key (should have received with the request)
         * is registered with the uuid.
         */
        void add_update_listener(request& r, const std::string& p);

        bool remove_update_listener(const std::string& u, const std::string& p);

        /**
         * @brief - Serializes update listeners into a file, for restoring them
         * after restarting Metax.
         */
        void serialize_update_listeners();

        /**
         * @brief - Sends to IMS confirmation that the uuid is found (locally
         * or remotely) and registration on update event is successful.
         */
        void send_ims_on_update_register_response(request& r);

        void send_get_livestream_fail_to_ims(const UUID& req, platform::default_package pkg);
        void clean_up_hosted_livestream_listeners(std::vector<ID32>& listeners);
        void clean_up_request_from_livestream_listeners(request& r);

        /**
         * @brief - Sends to remote peer confirmation that the uuid is found
         * (locally or remotely) and registration on update event is
         * successful.
         */
        void post_router_on_update_register_response(request& r);

        void handle_router_on_update_unregister_fail();
        void send_ims_on_update_unregister_response(request& r);
        void process_router_on_update_unregister(
                        const kernel_router_package& in);
        void post_router_authentication_request(request& r);
        void handle_storage_positive_response();

        /**
         * @brief - Handles case when a UUID is found in the storage and the
         * request is from local IMS or Kernel
         */
        void handle_storage_positive_response_for_local_request(
                                                        request&);

        /**
         * @brief - Handles case when a UUID is found in the storage and the
         * request remote peer
         */
        void handle_storage_positive_response_for_remote_request(
                                                        request& r);

        void handle_storage_negative_response();
        void send_ims_requested_data(request& req);
        void send_ims_response(const ims_kernel_package&);
        void send_ims_save_confirm(request& req);

        /**
         * @brief - Adds entry to user json and stores encryption keys
         */
        void add_new_owned_uuid_to_user_json(request& r,
                                        const save_unit& u);

        void update_owned_uuid_in_user_json(request& r);
        void update_copy_count_of_owned_uuid_in_user_json(
                        const Poco::UUID& uuid, int count);
        void handle_storage_negative_response_for_get_req(request& req);
        void handle_storage_negative_response_for_copy_req(request& req);
        void remove_owned_uuid_in_user_json(request& r);

        /**
         * @brief - Send requested data from storage.
         *
         * @param req - the request object
         */
        void send_router_requested_data(request& req);
        void send_ims_share_confirm(request& req);

        /**
         * @brief handles response for data lookup request in my storage
         *
         * @details If the request type is t_ims that means shared object was
         * requested. If the type is t_kernel then master node or a piece is
         * requested. t_router means no interest for us which node is requested.
         * If type is t_backup then we receiving shared object, and backup
         * process for the shared, master and piece nodes should be started.
         */
        void handle_storage_get_response();

        /**
         * @brief - Send metax::get command to router for given request.
         *
         * @param req - the request object for which get command should be send
         * to router.
         */
        void post_router_get_request(request& req);
        void post_router_refuse(request& r);
        void post_router_no_permission();
        void post_router_save_offer_request(ID32);
        void post_router_save_stream_offer(const request& r);
        void post_router_clear_request(ID32 req_id);
        void start_getting_pieces(request& r);

        void post_get_request_for_streaming_pieces(request& pid);

        /**
         * @brief - Send metax::save_data command to the storage for given
         * request object, UUID is taken from request object
         *
         * @param r - the given request object.
         */
        void post_storage_save_data_request(request& r);

        /**
         * @brief - Send metax::save_data command to the storage for given
         * request object, but save with the provided uuid
         *
         * @param r - the given request object.
         * @param u - uuid under which the data should be saved
         */
        void post_storage_save_data_request(request& r, const Poco::UUID& u);

        /**
         * @brief - The new version of the above function
         * TODO provide appropriate description
         *
         */
        void post_storage_save_request(ID32 i, platform::default_package pkg,
                                                const Poco::UUID& uuid);

        void post_storage_save_request(platform::default_package pkg,
                                                        const Poco::UUID& uuid);

        void post_storage_save_request(platform::default_package pkg,
                                                        const std::string& uuid);

        /**
         * @brief - Sends data to key manager for encryption.
         *
         * @detail - If aes key and aes initialization vector arguments are
         * empty, key manager will generate one, encrypt the data with the
         * generated key/iv pair and return them encrypted by the user's public
         * key along with the encrypted data.
         * If the key/iv is given to the key manager, then it will encrypt the
         * data using the received key/iv
         *
         * @param r - request object containing necessary information for
         * processing the data encryption
         * @param pkg - data to be encrypted
         * @param k - AES key, if empty, key manager will generate new one
         * @param i - AES initialization vector, if empty, key manager will
         * generate new one
         */
        void post_key_manager_encrypt_request(request& r,
                        platform::default_package pkg,
                        const std::string& k, const std::string& i);

        /**
         * @brief - Sends data to key manager for encryption.
         *
         * @detail - This function is used to encrypt the next portion of the
         * series of data to be encrypted with the same aes key. It assumes that
         * key manager already has cipher created and associated with the
         * request id.
         *
         * @param r - request object containing necessary information for
         * processing the data encryption
         */
        void post_key_manager_stream_encrypt_request(request& r);

        /**
         * @brief - Sends data to key manager for decryption.
         *
         * @detail - This function is used to decrypt the next portion of the
         * series of data to be decrypted with the same aes key. It assumes that
         * key manager already has cipher created and associated with the
         * request id.
         *
         * @param r - request object containing necessary information for
         * processing the data decryption
         */
        void post_key_manager_stream_decrypt_request(request& r,
                        const platform::default_package& pkg);

        /**
         * @brief - Sends key menager request to generate aes key.
         *
         * @detail - key manager creates and remembers the cipher created based
         * on the generated aes key for further encrypting of the data received
         * with the same request id.
         * This is used in particular for encrypting livestream audio/video
         *
         * @param r - request object containing necessary information
         */
        void post_key_manager_gen_key_for_stream(request& r);

        /**
         * @brief - Sends key menager request to create cipher from the
         * provided aes key.
         *
         * @detail - key manager decrypt the received aes key, creates cipher
         * and remembers the for further encryption/decryption of the data
         * received with the same request id.  This is used in particular for
         * encrypting/decrypting livestream audio/video
         *
         * @param r - request object containing necessary information for
         * preparing the cipher
         */
        void post_key_manager_prepare_cipher_for_stream(request& r);

        /**
         * @brief Post decrypt request to key manager for given request object.
         *
         * @param req - request object which payload should be decrypted.
         * @param key - encrypted key string which should be send to key manager
         * for decrypt.
         * @param pkg - data to be decrypted
         * @param iv - encrypted iv string which should be send to key manager
         * for decrypt.
         */
        void post_key_manager_decrypt_request(request& req,
                        platform::default_package pkg,
                        const std::string& key, const std::string& iv);

        void construct_get_request_for_piece(const UUID& uuid,
                                                request& preq, ID32 size);

        void set_piece_key(request& req, request& preq);

        /**
         * @brief Creates new request for given uuid and sends storage lookup
         * request for it.
         *
         * @param uuid - uuid of piece which should be looked up.
         * @param preq - request object which should be set as parent request.
         */
        void post_look_up_request_for_piece(const UUID& uuid,
                                        request& preq, Poco::UInt64 size);

        ID32 get_request_id();
        void handle_storage_save_confirm();
        void handle_save_confirm_for_local_save_unit(save_unit& u, ID32 uid);
        void handle_no_space_for_local_save_unit(save_unit& u, ID32 uid);
        void handle_storage_no_space();
        void handle_no_space_for_copy_or_remote_request(request& r,
                        const kernel_storage_package& in);
        void handle_save_confirm_for_copy_or_remote_request(request& r);
        void handle_save_confirm_for_remote_recording_start(request& r);
        void handle_save_confirm_for_local_recording_start(request& r);
        void remove_from_backup_units(request& r, ID32 uid);
        void handle_failed_save_unit(save_unit& unit, ID32 uid,
                        const std::string& err);
        void handle_canceled_save_unit(save_unit& unit, ID32 uid,
                        const std::string& err);
        void handle_first_backup_of_save_unit(save_unit& u, ID32 req_id);
        void handle_authorized_get_request(request& r);
        void handle_router_copy_request();
        void post_storage_copy_request(request& req,
                        platform::default_package pkg);
        void handle_router_send_journal_info();
        void handle_router_get_journal_info(const kernel_router_package& pkg);
        void handle_router_sync_finished_request();
        void post_router_send_uuids_request(request& r);
        void handle_router_send_uuids_request();
        void handle_backup_get_uuid(const kernel_backup_package& in);
        void handle_backup_send_uuids(const kernel_backup_package& in);
        void handle_router_get_uuid();
        void add_new_uuid_in_device_json(const request& req,
                        const Poco::UUID& uuid, uint32_t version);
        void add_new_uuid_in_device_json(const kernel_router_package& in);
        void post_router_sync_finished(ID32 rid);
        void process_journal_info_for_sync(const kernel_router_package& in);
        void handle_failed_sync_request(request& req);
        void handle_file_decryption_fail();
        bool is_authorized_request(ID32 req_id) const;
        bool should_wait_for_user_json(command cmd) const;
        bool is_external_request_authorized(const kernel_router_package&) const;
        void handle_storage_input();
        void handle_cache_input();
        void handle_storage_writer_input();
        void process_storage_input();
        void process_cache_input();
        void process_storage_writer_input();
        void handle_wallet_input();
        void process_wallet_input();
        void handle_router_get_request();
        void handle_router_get_data();
        void handle_router_find_peer();
        void handle_router_save_request();
        void handle_router_update_request();
        void handle_router_save_stream_request();
        void handle_router_recording_start();
        void handle_router_recording_stop();
        void stop_recording_and_send_response_to_router(livestream_info& li);
        void handle_router_save_data();
        void handle_router_save_offer();

        /*
         * @brief handles save stream offer from router.  Now the first offer
         * is unconditionally accepted by the kernel
         */
        void handle_router_save_stream_offer();

        /*
         * @brief handles save stream offer from router.
         */
        void handle_router_livestream_content();

        /*
         * @brief Sends livestream content (unencrypted of after key manager
         * has unencrypted to IMS.
         */
        void send_livestream_content_to_ims(UUID uuid,
                        platform::default_package);

        void handle_router_save_confirm();
        void handle_router_auth_request();
        void handle_router_auth_data();
        void handle_router_cancel();
        void handle_router_input();
        void process_router_input();
        void process_router_pending_get_requests();
        void handle_create_request();

         /**
          * @brief - Entry point of save algorithm when the save request
          * received from IMS
          *
          * @detail - Reads all information from the RX channel of IMS,
          * stores information in a request object in m_requests
          * container, and starts processing of the save request
          */
        void handle_ims_save_request();

         /**
          * @brief - Handles save_stream request received from IMS, which is
          * going to be a livestream (video/audio)
          */
        void handle_ims_save_stream_request();

         /**
          * @brief - Handles livestream content received from IMS. Either sends
          * the content to key_manager for encryption or sends the content to
          * listenders.
          * going to be a livestream (video/audio)
          */
        void handle_ims_livestream_content();

        void handle_ims_delete_request();
        void process_delete_request(request& r);
        void post_router_delete_request(request& r);
        void handle_storage_deleted_response_for_kernel_request(request& r);
        void handle_deleted_response_for_kernel_request(request& r);
        void handle_deleted_response_for_ims_request(request& r);
        void process_failed_delete_request(request& req);
        void handle_decryption_fail_for_delete_request(request& req);
        void handle_storage_deleted_response_for_ims_request(request& r);

        /**
         * @brief - Receives device public key from IMS
         * and sends them to configuration manager to add as a new trusted peer
         */
        void handle_ims_add_trusted_peer();

        /**
         * @brief - Receives peer device public key from IMS
         * and sends them to configuration manager to remove trusted peers
         */
        void handle_ims_remove_trusted_peer();

        /**
         * @brief - Retreives trusted peers name from configuration manager
         */
        void handle_ims_get_trusted_peer_list();

        /**
         * @brief - Receives friend name and public key from IMS
         * and sends them to configuration manager to add as a new friend
         */
        void handle_ims_add_friend();

        /**
         * @brief - Retreives all friends names and public keys from
         * configuration manager in json format
         */
        void handle_ims_get_friend_list();

        /**
         * @brief - Receives friend's username from IMS,
         * and sends the username to Configuration Manager
         * for getting friend's public key.
         */
        void handle_ims_get_friend();

        /**
         * @brief - Passes the command to Storage
         */
        void handle_ims_move_cache_to_storage();

        /**
         * @brief - Cleans request in the tender (streaming get) when IMS
         * informs that the request not needed anymore, as connection with the
         * client is over.
         */
        void handle_ims_clean_stream_request();

        /**
         * @brief - processes register on update request from IMS.
         * Creates register object and stores all necessary information for the
         * request there, then asks key manager to provide user public key.
         */
        void handle_ims_register_on_update_request();

        void handle_ims_unregister_on_update_request();

        /**
         * @brief Cleanup interrupted or finished livestream request
         */
        void handle_ims_livestream_cancel();

        void handle_ims_cancel_livestream_get();

        /**
         * @brief - checks whether there is a value in rx channel of IMS
         * connection. In case of data existence, checks the request type (cmd
         * variable) received from IMS and calls corresponding function to
         * process the request.
         */
        void handle_ims_input();

        void process_ims_input();
        void handle_key_manager_input();
        void process_key_manager_input();

        void handle_config_send_to_peer();

        /**
         * @brief - Receives from Configuration Manager friend's public key
         * and sends it to IMS (with IMS specific request_id).
         */
        void handle_config_get_friend_found();

        void handle_kernel_cfg();

        /**
         * @brief - Receives from Configuration Manager that the specified
         * username's public key isn't available.
         */
        void handle_config_get_friend_not_found();

        /**
         * @brief - Receives from Configuration Manager that the specified
         * device public key successfully added to the list of
         * trusted peers.
         */
        void handle_config_add_trusted_peer_response();

        /**
         * @brief - Receives from Configuration Manager that the specified
         * device public key successfully removed from the list of
         * trusted peers.
         */
        void handle_config_remove_trusted_peer_response();

        /**
         * @brief - Receives from Configuration Manager that the specified
         * username and it's public key successfully added to the list of
         * friends.
         */
        void handle_config_add_friend_response();

        /**
         * @brief - Receives trusted peers list from configuration manager and
         * passes to ims
         */
        void handle_config_get_trusted_peer_list_response();

        /**
         * @brief - Receives friend list from configuration manager and
         * passes to ims
         */
        void handle_config_get_friend_list_response();

        void handle_config_get_public_key_response();

        void handle_key_manager_get_user_keys_response(
                        const kernel_key_manager_package& in);
        void clean_up_save_request(save_unit& u, ID32 uid);
        void handle_first_copy_success_for_master_object(request& r);
        void handle_first_copy_success_for_piece(request& r, request& preq);

        void send_ims_get_public_key_response(request& r);

        void post_router_on_update_register_request(request& r);

        void post_router_on_update_unregister_request(request& r);

        /**
         * @brief - Handles case when  path of update listeners received from
         * config manager. Parses the file (if exists) and stores the info in a
         * member.
         */
        void handle_config_update_listeners_path();

        /**
         * @brief - Receives the path of user json via message payload.
         */
        void handle_decrypted_key(request& r);

        void send_update_to_user_manager(command cmd, const Poco::UUID u,
                                        const std::string& iv = "",
                                        const std::string& key = "",
                                        int copy_cnt = 1);

        /**
         * @brief - Receives the path of user json via message payload.
         */
        void add_shared_info_in_user_json(request& r,
                        const std::string& key,
                        const std::string& iv, unsigned int copy_count = 1);

        /**
         * @brief - Receives the path of user json via message payload.
         */
        void process_router_save_stream_offer_for_encrypted_stream(request& r);

        /**
         * @brief - Receives the path of user json via message payload.
         */
        void post_save_stream_offer_to_ims(request& r);

        void handle_key_decryption_fail(request& req);

        /**
         * @brief - Checks whether the user json has correct format.
         * Throws exception if it is not
         */
        bool is_valid_user_json();

        void handle_config_input();
        void handle_backup_input();
        void handle_user_manager_input();
        void process_user_manager_input();
        void process_config_input();
        void process_backup_input();
        void post_router_get_journal_info();
        void handle_canceled_ims_request(request& req);
        void handle_canceled_router_request(request& req);

        void add_livestream_chunk_to_all_recordings(livestream_info& li);
        void add_livestream_chunk_to_recording(livestream_info& li,
                        std::vector<recording_info>::iterator ri,
                        const Poco::UUID& uuid);
        void clean_up_hosted_livestream(request& req);
        bool clean_livestream_get_request_from_pendings(const UUID&, ID32&);
        void handle_canceled_ims_get_request(request& req);
        void handle_canceled_kernel_request(request& req);

        /**
         * @brief - Sends a message to IMS for forwarding through websocket.
         *
         * @param m - the message
         * @param id - a request ID. IMS sends it back in confirmation (or fail
         * message)
         */
        void send_notification_to_ims(ID32 id,
                        platform::default_package m);

        void send_notification_to_user_manager();

        void handle_user_manager_on_update_register(const Poco::UUID& uuid);
        void send_notification_to_router(request& r,
                                platform::default_package m,
                                metax::command cmd, uint32_t v);

        /**
         * @brief - Sends update event to the registered peers
         *
         * @param u - The updated UUID
         */
        void notify_update(const Poco::UUID& u, metax::command cmd, uint32_t v);

        /**
         * @brief - Handles failed ims and kernel get request.
         */
        void handle_failed_get_request(request& req);

        /**
         * @brief - Handles negative response for the given get request,
         * if there is a missing chunk in the requested object.
         */
        void handle_failed_kernel_get_request(request& req);

        /**
         * @brief - Handles negative response for the given get request,
         * if the uuid in the valid format does not exist in the storage
         * or the requested uuid is not a valid shared object.
         */
        void handle_failed_ims_get_request(request& req);

        void handle_failed_share_request(request& req);

        /**
         * @brief - Handles negative response for the given update request,
         * if the uuid in the valid format does not exist in the storage.
         */

        void send_ims_update_fail_response(request& req, const std::string& err);

        /**
         * @brief Handle storage negative response for given ims request.
         * For metax::get requests posts get request to router.
         * Other cases is not handled yet.
         *
         * @param r - request object.
         */
        void handle_storage_negative_response_for_ims_request(request& r);

        void handle_failed_send_to_peer_request(ID32);

        /**
         * @brief - handles failure of on update register request, e.g. the
         * requested uuid is not found.
         */
        void handle_failed_on_update_register_request(request& r,
                        const std::string& err = "");

        void handle_failed_on_update_unregister_request(request& r,
                        const std::string& err);
        void handle_key_manager_reencrypted_key_request();

        /**
         * @brief - Handles response of encrypt requests to key manager.
         *
         * @detail - The encrypted data will be in either payload or in file.
         * That can be checked by file_path member (empty or not) of the
         * received message
         */
        void handle_key_manager_encrypted_request();

        void handle_key_manager_decrypted_request();

        /**
         * @brief - processes chunk when it is received unencrypted form
         * storage/router, of decrypted by key manager during get request.
         *
         * @detail - the chunk is streamed if it is a chunk of a media file or
         * is written into a corresponding bytes of a whole file.
         *
         * @param pkg - chunk content
         * @param preq - parent request (metax::get)
         * @param uuid - the uuid of the chunk
         */
        void process_unencrypted_piece(const platform::default_package pkg,
                                request& preq, const Poco::UUID& uuid);
        void handle_failed_request(request& req);
        void handle_ims_update_request();
        void handle_router_authorized_request();

        /**
         * @brief processes received master json, which is either received from
         * storage unencrypted or received from key manager after decrypt
         *
         * @param pkg - master json in string representation
         * @param preq - parent request which should be either get or update
         * @param rid - id of the current request, get or update
         */
        void process_unencrypted_master_json(request& preq);

        void handle_key_manager_id_found_request();
        void post_router_peer_found(request& r);
        void send_update_notification_to_ims(request& r, const std::string& e);
        void handle_key_manager_id_not_found_request();
        void handle_ims_share_request();
        void handle_ims_accept_share_request();
        void handle_ims_reconnect_to_peers();
        void handle_ims_start_pairing();
        void handle_ims_cancel_pairing();
        void handle_ims_get_pairing_peers();
        void handle_ims_request_keys();
        void handle_ims_recording_start();
        void post_router_recording_start(request& r);
        void post_router_recording_stop();
        void create_and_save_master_for_recording(request& r);
        void create_and_save_playlist_for_recording(request& req,
                        recording_info& r, const Poco::UUID& rec_uuid);
        void handle_ims_recording_stop();
        void stop_recording_and_send_response_to_ims(livestream_info& li);
        void post_find_peer_request(ID32 req_id,
                        const platform::default_package&);
        void handle_ims_send_to_peer_request();
        void handle_router_peer_found(ID32 id);
        void handle_router_send_to_peer(ID32 id);
        void handle_router_send_to_peer_confirm(ID32 id);
        void post_send_to_peer_confirm(ID32 id);

        /**
         * @brief Confinues save algorithm when file encryption is completed by
         * key manager
         *
         * @detail either the data or master json encryption is arrived.
         */
        void handle_save_request_after_encrypt(request&);

        /*
         * @brief handles cases when: 1) Aes key is generated and encrypted for
         * livestream, 2) livestream content is encrypted by key_manager
         */
        void handle_encrypted_save_stream(request&);

        /*
         * @brief Sends livestream chunk to router received either from ims or
         * from key manager (after encryption)
         */
        void send_livestream_content_to_router(request&,
                                platform::default_package, uint32_t orig_size);

        /**
         * @brief - If the received chunk is part of streaming then send the
         * chunk to IMS and update the streaming state. Otherwise write in the
         * prepared file in the permanent memory in the appropriate byte range.
         */
        void handle_piece(const platform::default_package& in, request& req);

        void handle_piece_for_backup(ID32 i, save_unit& u, request& preq);

        /**
         * @brief When metax::encrypted message arrives from key manager (KM),
         * the kernel detects whether KM encrypted a file, or a master json
         *
         * @detail when KM encrypts a file for the first time, it returns also
         * the encrypted AES key and initialization vector (IV). The existance
         * of the key and iv is checked to determine the condition.
         */
        bool encrypted_master_json_for_save(
                        const kernel_key_manager_package&);

        bool should_generate_new_key(request& r);

        /**
         * @brief Continue save algorithm after received encrypted file (or data
         * in memory) from key manager
         *
         * @detail - prepares share object and master all the pieces (of
         * already encrypted data) ready for save by creating corresponding
         * save_unit objects and stores them into the m_pending_save_requests
         * container. Constructs master json and sends it to key manager for
         * encryption.
         *
         * @param r - request object containing data for save request.
         */
        void handle_save_after_piece_encrypt(request& r);

        /**
         * @brief - Creates share object, fills save_unit object with all
         * necessary data to perform save of share object and places the
         * save_unit object in m_pending_save_requests list
         *
         * @param r - request object of original save request.
         * @param mjuuid - uuid of the master json
         * @param key - AES key. Can be empty if save request is unencrypted.
         * @param key - AES initialization vector. Can be empty if save request
         * is unencrypted.
         */
        void prepare_shared_object_for_save(request& r, const UUID& mjuuid,
                        const std::string& key, const std::string& iv);

        /**
         * @brief - When master node of streaming get arrives and streaming
         * state is constructed, all other get requests of the same uuid are
         * updated to share the same streaming state.
         */
        void update_pending_stream_requests(request& req);

        /**
         * @brief - Creates request with the gived id and other info, which is
         * used to send encrypt request to key manager. It is used for
         * encrypting master json or a piece of saved data.
         */
        request& create_request_for_unit_encrypt(
                                        ID32 i, platform::default_package pkg,
                                        const Poco::UUID& uuid, request& pr);

        /**
         * @brief - Creates request to used for sending update notifications to
         * registered peers. Also constructs the text of the update
         * notification message.
         */
        leviathan::metax::kernel::request&
                create_request_for_update_notification(
                                const Poco::UUID& u, metax::command cmd);

        /**
         * @brief - Sorts the list of chunk_streaming_info object according to
         * the start byte address of the chunks.
         */
        void sort_streaming_chunks(
                        std::vector<chunk_streaming_info>& v);

        void set_encryption_keys_if_mls(request& req);

        /**
         * @brief - When master node arrives for a streaming get requests,
         * this function updates all the requests streaming states also waiting
         * for the same uuid.
         */
        void construct_streaming_state(request& req);

        void construct_streaming_state_from_pieces(request& req);
        void construct_streaming_state_from_content(request& req);

        /**
         * @brief - Checks whether the data received as payload or file and
         * returns its size.
         */
        Poco::UInt64 get_data_size_from_request_object(request& r);

        /**
         * @brief - Constructs "pieces" element for adding to
         * master json.
         *
         * @detail - The splitting in the chunks depends on the value of
         * max_chunk_size config. If the size of the whole data is less
         * then the value of the config then the data will not be split into
         * chunks
         *
         * @param s - the size of the whole data to be saved.
         */
        Poco::JSON::Object::Ptr generate_pieces_json(Poco::UInt64 s);

        /**
         * @brief - Core save algorithm when there is no encryption requested.
         *
         * @detail - creates share object json and master json. Generates
         * save_unit objects, which will store necessary information for
         * processing save requests of share object, master json and all the
         * pieces of the data. The save_unit objects are placed in a
         * m_pending_save_requests list. And at end starts sending of save
         * requests to storage and router from m_pending_save_requests
         * list.
         *
         * @param r - request object containing all the necessary data to
         * perform save request
         * @param mj - master json in string
         */
        void handle_save_of_unencrypted_data(request& r);

        /**
         * @brief - Prepares share object, master json and pieces of data for
         * saving.
         *
         * @detail - places the save unit object in m_save_units,
         * m_pending_save_requests containers and units set of request object.
         * m_pending_save_requests are used to send save requests to storage
         * and router.
         * m_save_units are used to track the responses of the save requests
         * for each unit (share object, master json, pieces). Also keeps track
         * of backup counts
         * request::units is to ensure that each unit has at least one
         * confirmed response from either storage or router in order to send
         * response to IMS.
         *
         * @param r - request object of the save request
         * @param b - Start byte of the data pieces. For share object and
         * master json it is 0
         * @param s - Size of the unit (share object, master json, pieces)
         * being saved.
         * @param t - The type of the unit (share or master or piece)
         * @param uuid - UUID of the unit
         * @param c - The unit data itself. For pieces it is empty as the
         * actual data will be read just before sending the requests to storage
         * and router
         */
        ID32 prepare_unit_for_save(request& r, unsigned int b, Poco::UInt64 s,
                                        save_unit::unit_type t,
                                        const UUID& uuid,
                                        platform::default_package c,
                                        bool backup = false);

        /**
         * @brief - Creates save_unit objects for file chunks, and adds the
         * objects to m_pending_save_requests container.
         */
        void prepare_pieces_for_save(request& r);

        /**
         * @brief - processes the received master node (from storage, router or
         * key manager)
         *
         * @detail - If the file is media (audio or video) then initializes
         * information for keeping track on streaming state. Otherwise prepares
         * storage on permanent memory.
         * Starts posting get requests to storage for all the pieces.
         */
        void handle_decrypted_master_node_for_get(request& req);

        void send_ims_playlist_for_recording(request& req);

        /**
         * @brief - When shared object arrives for a get request and the
         * content type is audio or video, kernel iforms IMS that the get will
         * be done through streaming, so that IMS can be prepared for
         * subsequent chunk sending for this request.
         */
        void process_stream_prepare(request& req);

        uint32_t get_piece_size(Poco::UInt64 size) const;

        /**
         * TODO - comment should be updated.
         * @brief Handle shared object which can be received from storage or
         * router. For metax::update command updates the content of master
         * node, for metax::get command lookups master node.
         * Checks the content type of the file in shared object and if it is a
         * media type (audio or video) then allocates object for keeping track
         * of the streaming state and keeps in the request object.
         *
         * @param pkg - default_package which contain shared object data.
         * @param req - request object for which shared object data should be
         * handled.
         */
        void got_master_object(const platform::default_package& in, request& req);

        void handle_unencrypted_master_object(request& req,
                        platform::default_package pkg);
        void parse_master_object(request& req, platform::default_package pkg);
        void handle_get_request_after_decrypt(request& r);
        Poco::JSON::Object::Ptr
                get_master_info_from_user_json(const UUID& uuid) const;
        Poco::JSON::Object::Ptr
                get_master_info_from_user_json(const std::string& uuid) const;
        void get_keys_from_master_info(request& req,
                        Poco::JSON::Object::Ptr u);

        /**
         * @brief - Validates given json object for shared object.
         * Checks the existence of "aes_key", "aes_iv", "master_uuid" fields
         * in given json object.
         *
         * @param obj - json object which should be validated.
         *
         * @return Returns true if all needed fields exists in given json, false
         * otherwise.
         */
        bool is_valid_shared_object(const Poco::JSON::Object::Ptr& obj) const;
        /**
         * @brief - Whether the content type of the file which is being
         * retreived from sotrage/router is audio or video
         *
         */
        bool is_streaming_media(request& req);

        /**
         * @brief - Validates given json object for master node.
         * Checks the existence of "pieces", "size" fields in given json object.
         *
         * @param obj - json object which should be validated.
         *
         * @return Returns true if all needed fields exists in given json, false
         * otherwise.
         */
        bool is_valid_master_node(const Poco::JSON::Object::Ptr& obj) const;

        Poco::UInt64 get_start_position(request& req, const UUID&);
        Poco::JSON::Object::Ptr parse_json_data(
                        platform::default_package pkg) const;
        Poco::JSON::Object::Ptr
                create_piece_json(Poco::UInt64, uint32_t);
        void process_ims_get_request(request& r);
        void post_router_stream_auth_request(const request& r);
        void process_hosted_livestream_get_request(request& r,
                        std::unique_ptr<livestream_info>& li);

        /**
         * @brief - Updates streaming state of the new_req from the orig_req
         * which has already constructed streaming state, while they are
         * getting the same uuid.
         * Sends streaming prepare command to IMS.
         */
        void set_streaming_state(request& orig_req, request& new_req);

        /**
         * @brief - calculates the chunk number from the start byte when range
         * get request is received from IMS.
         */
        void calculate_stream_current_chunk_from_range(request& req);

        void normalize_streaming_range(request& req);

        /**
         * @brief - Entry poind of get request from Vostan IMS.
         *
         * @detail - Reads all information concerning the request from IMS rx
         * channel and stores in request object, then sends the file lookup
         * request to own storage.
         * Note, if another get request from IMS for the same uuid is already
         * in processing, then the lookup request to stroge is not sent,
         * instead the request is associated with the uuid in a separate map.
         * Once the the uuid is received, the file will be sent as response to
         * ALL IMS requests for the uuid
         */
        void handle_ims_get_request();

        void lookup_master_node(const UUID& in, const request& preq);
        void process_share_request(request& req);
        void process_ims_accept_share_request();
        request& create_master_node_request(request& , const UUID&);
        void save_master_object(request& r,
                platform::default_package pkg);

        /**
         * @brief - Creates the json template of master json without filling
         * the "pieces" the element. After the call of this function the
         * "pieces" element should be added make the master json valid.
         * The master structure is:
         * {
         *         "pieces" : {
         *              "<piece uuid>" : {
         *                      "from" : <start byte>,
         *                      "size" : <piece data size>
         *              },
         *              ....
         *         },
         *         "size" : <whole data size>,
         *         "version" : <version number>,
         *         "created": <created date>,
         *         "last_updated": <last updated date>,
         *         "expiration_date": <date of expiration>
         * }
         */
        Poco::JSON::Object::Ptr
                create_master_json_template(const request& r, Poco::UInt64 size,
                                const std::string& ct);

        void get_piece_data_for_save(save_unit& u);
        leviathan::platform::default_package get_piece_data_for_save(
                        const request& r, uint64_t start, uint64_t size);
        void prepare_file_write(Poco::JSON::Object::Ptr master,
                        const std::string& path);

        /**
         * @brief - writes the date into the provide tmp file
         * file.
         *
         * @param data - data to be written in temporary file
         * @param rmp - path to tmp file
         */
        void write_data_in_tmp(const std::string& tmp,
                        const platform::default_package& data);

        Poco::TemporaryFile* prepare_partial_chunk(request& preq);

        /**
         * @brief - Sends the current chunk to IMS, if the next chunks are
         * already available then they are also being sent to IMS, till the
         * stream vector is handled completely or there is a not available
         * chunk.
         */
        void send_available_chunks_to_ims(request& preq,
                                                const UUID& uuid);

        /**
         * @brief - Finds the index of the chunk in streaming chunk vector
         * (ordered by starting bytes)
         */
        Poco::UInt64 get_streaming_chunk_index(request& preq,
                                                const UUID& uuid);

        void write_data_to_stream(const platform::default_package& data,
                         request& req, const UUID& uuid);

        /**
         * @brief - Finds the index of the chunk in the chunk list.  If it is
         * the current chunk kernel is waiting then it is sent to IMS.
         * Otherwise the chunk is being written in temporary file and the
         * streaming state is updated by marking the chunk as received and
         * available to be sent when its turn comes. If all the chunks are sent
         * to IMS, then the request is cleaned up.
         *
         */
        void handle_streaming_chunk(const platform::default_package& data,
                        request& req, const UUID& uuid);

        void post_storage_next_piece_get_request(request& req);

        void post_storage_next_streaming_chunk_get_request(request& req);

        /**
         * @brief When a chunk arrives during streaming get request sends the
         * chunk to all ims requests waiting for the chunk. Also sends the next
         * chunks if they are available.
         */
        void process_pending_get_streaming_requests(
                                request& preq, Poco::UInt64 i);

        /**
         * @brief - Starts processing of save request received from IMS.
         * Checks whether the encryption requested and chooses the
         * corresponding algorithm. All necessary data for processing save
         * request is taken from request object which receives as argument
         *
         * @param r - request object, containing all necessary information from
         * IMS to perform processing of the save request.
         */
        void process_ims_save_request(request& r);

        void process_ims_update_request(request& req);
        void print_debug_info() const;

        /**
         * @brief - Reduces the ram usage counter when there is a save or update
         * request is completed (or failed to complete) and ram is freed from
         * the corresponding content.
         */
        void reduce_router_save_input_size(uint32_t s);

        void reduce_router_get_input_size(uint32_t s);
        void reduce_storage_input_size();
        void backup_data(ID32 id, const Poco::UUID& uuid, metax::command cmd,
                uint8_t copy_cnt, platform::default_package pkg,
                uint32_t version, const request& preq);

        /**
         * @brief - Handle save and backup process for pieces.
         */
        void handle_piece_save(ID32 id, platform::default_package pkg,
                                const Poco::UUID& uuid, request& pr);

        /**
         * @brief handle the case and remote peer sends auth request in
         * response of update request (master json or shared object)
         */
        void handle_update_auth_request(save_unit& u);

        void handle_router_no_permissions();

        /**
         * @brief - Checks resources used by the pending save requests didn't
         * take all the resources (RAM, network bandwidth, etc.) given to
         * Metax. If not then takes each from m_pending_save_requests by turn
         * and posts save requests to router and storage. Checks for the
         * resource limits upon processing of each element from the list.
         */
        void process_pending_save_requests();
        void process_storage_pending_save_requests();
        void process_router_pending_save_requests();
        void handle_canceled_ims_save_stream_request(request& req);
        void handle_recording_fail(request& req, platform::default_package m);
        void handle_canceled_ims_recording_start(request& req);
        void handle_canceled_ims_recording_stop(request& req);
        void handle_livestream_content_for_ims_requests(
                        std::unique_ptr<livestream_info>& li);
        void handle_livestream_content_for_router_request(
                        std::vector<ID32>& listeners);
        void add_content_to_recording_chunk(livestream_info& li,
                        platform::default_package d, uint32_t orig_size);
        void send_get_livestream_content_to_router(request& r,
                        platform::default_package pkg);

        /**
         * @brief - Posts delete request to storage for uuid of specified
         * request object.
         *
         * @param r - request object which uuid should be deleted.
         */
        void post_storage_delete_request(request& r);

        /**
         * @brief - Handles storage deleted response for requests with t_kernel
         * and t_router types. If request type is t_kernel than metax::del
         * request sends to router. If request type id t_router than
         * metax::deleted response sends to router and refuses the delete
         * request to router.
         */
        void handle_storage_deleted();

        void handle_storage_deleted_for_external_request(request& r);

        void handle_move_cache_to_storage_completed();

        void handle_failed_delete_request(request& req);

        /**
         * @brief - Handles metax::del request received from router.
         * Sends look up request to storage for specified uuid.
         */
        void handle_router_delete_request();

        /**
         * @brief - Handles metax::deleted response from router. Cleans up
         * request if receives metax::deleted response max_save_count
         * times.
         */
        void handle_router_deleted();

        /**
         * @brief - Handles metax::on_update_register request from router.
         * Posts storage lookup request for the received UUID for the further
         * processing of the request.
         */
        void handle_router_on_update_register();

        void handle_router_on_update_unregister();

        void handle_router_on_update_unregistered();

        /**
         * @brief - Handles metax::on_update_registered response from router.
         * Forwards the message to IMS.
         */
        void handle_router_on_update_registered();

        /**
         * @brief - Handles livestream content from peer. Sends the content to
         * ims (if non-encrypted stream) or sends to key manager for decryption
         */
        void handle_router_get_livestream_content();

        void handle_router_cancel_livestream_get();
        void handle_router_recording_started();
        void send_ims_recording_started_response(
                        request& r, const Poco::UUID& u);
        void handle_router_recording_stopped();
        void handle_router_recording_fail();
        void handle_router_stream_auth_request();

        /**
         * @brief - Creates metax::del request for uuids specified in old_uuids
         * member and sends metax::look_up request to storage.
         */
        void remove_unnecessary_uuids(request& r);

        /**
         * @brief - Handles storage negative response for update requests.
         * If request type is t_ims that means shared object is not found in
         * local storage and metax::get request should be send to router
         * If request type is t_kernel that means old master json missing
         * in local storage.
         *
         * @param req - request object for witch handles storage negative
         * response.
         */
        void handle_storage_negative_response_for_update_req(request& req);

        void handle_storage_negative_response_for_delete_req(request& r);

        /**
         * @brief - Handles storage negative response for on_update_register
         * requests. Get's the user public key for forwarding the request to
         * peers.
         */
        void handle_storage_nack_for_on_update_register_req(request& req);

        void post_config_get_public_key_request(request& req);

        void post_storage_add_cache_request(request& req);
        void post_storage_remove_from_cache_request(const UUID& req);
        void post_router_save_confirmed_response(request& req);
        void handle_router_failed();
        void handle_ims_failed();
        void handle_ims_send_to_peer_confirmed();
        void handle_key_manager_failed_get_request(request&);

        /**
         * @brief - Handles metax::decryption_fail request received from
         * key_manager.
         */
        void handle_key_manager_failed_share_request();

        void handle_key_manager_decryption_fail_request();
        void handle_key_manager_stream_decrypt_fail();
        void process_router_save_stream_request(request& r);
        void handle_ims_get_user_public_key();
        void handle_ims_get_user_keys();
        void handle_ims_get_online_peers();
        void handle_ims_get_metax_info();
        void handle_ims_set_metax_info();
        void handle_config_get_online_peers_response();
        void handle_config_get_pairing_peers_response();
        void handle_config_get_generated_code();
        void handle_key_manager_request_keys_response(
                        const kernel_key_manager_package&);
        void handle_request_keys_fail(ID32 request_id,
                        platform::default_package in);
        void handle_key_init_fail();
        void handle_config_get_metax_info_response();
        void handle_config_set_metax_info_ok();
        void handle_config_set_metax_info_fail();
        void handle_ims_copy_request();
        void process_copy_request(request& r);
        void handle_copy_success_for_master_object(request& r);
        void handle_copy_success_for_piece(request& r);
        void handle_copy_success_for_remote_request(request& r);
        void handle_storage_copy_succeeded(ID32 req_id);
        void handle_storage_no_space_copy();
        void handle_storage_failed(const kernel_storage_package& in);
        void handle_router_copy_succeeded(ID32 req_id);
        void remove_id_from_parent_units(request& preq, ID32 req_id);
        void handle_master_object_save_offer_for_copy_request(request& r);
        void handle_failed_copy_request(request& req);
        void generate_copy_request_for_pieces(request& r);
        void handle_copy_of_master_object(request& r);
        void handle_copy_of_encrypted_master_object(request& r);
        void handle_copy_of_each_piece(request& r, const std::string& old_uuid,
                          const std::string& new_uuid);
        void post_router_copy_request(request& r);
        void send_ims_copy_failed_response(request& req, const std::string&);
        void handle_failed_kernel_copy_request(request& req);
        void add_new_piece_of_copy_req_in_user_json(request& r);
        void mark_uuid_as_deleted_in_device_json(const Poco::UUID& uuid,
                uint32_t dversion, Poco::UInt64 last_updated);
        void handle_share_accept_for_public_data(request& r, ID32 ims_id);
        uint32_t increase_version(Poco::JSON::Object::Ptr obj) const;
        void send_ims_get_fail_request();
        void process_requested_uuid_for_sync(request& req,
                        const kernel_router_package& in);
        void update_info_for_deleted_uuid(const std::string& uuid,
                                const std::string dversion,
                                const std::string last_updated);
        void process_save_confirm_for_master_object(save_unit& u);
        void start_master_object_save(request& r);
        void process_save_confirm_for_piece(save_unit& u, ID32 uid);
        void handle_key_manager_get_aes_key();
        void process_save_request_after_key_generation(request& r);
        void process_not_backed_up_save_unit(request& r,
                        const std::string& err);
        void send_ims_save_fail_response(request& r, const std::string& err);
        ID32 handle_piece_delete(const Poco::UUID& uuid, ID32 preq_id);
        void send_ims_copy_succeeded_response(request& preq);
        void handle_router_on_update_register_fail();
        void send_dump_user_info_succeeded_response_to_ims(request& req);
        void handle_ims_dump_user_info();
        void send_dump_user_json_request(ID32 req_id,
                       const std::string& key = "", const std::string& iv = "");
        void send_dump_device_json_request(ID32 req_id,
                       const std::string& key = "", const std::string& iv = "");
        void handle_user_info_dump_confirm(ID32 req_id);
        void initialize_user_json();
        void backup_user_json();
        void get_user_json_from_peer();
        void handle_ims_regenerate_user_keys();
        void handle_regenerate_user_keys_succeeded();
        void handle_user_keys_regeneration_fail();
        void process_received_master_object_from_router(
                        const platform::default_package& in, request& req);
        void handle_failed_ims_copy_request(request& req);
        void send_ims_delete_failed_response(request& req);
        void handle_failed_ims_update_request(request& req,
                        const std::string& err);
        void handle_router_get_data_for_user_manager_request(
                        const kernel_router_package& in);
        void send_ims_stream_prepare(request& req, ID32 id);
        void handle_router_save_confirm_for_request_object(request& r);
        void handle_save_confirm_for_user_json(request& r);
        void handle_canceled_user_manager_request(request& req);
        void handle_user_json_get_fail(request& req);
        void handle_user_json_save_fail(request& req);
        void handle_user_json_updated_response(request& r);
        std::string get_trusted_peer_list();

        /// @name Public data members
public:
        KEY_MANAGER_RX key_manager_rx;
        STORAGE_RX storage_rx;
        STORAGE_RX storage_writer_rx;
        STORAGE_RX cache_rx;
        INPUT wallet_rx;
        IMS_RX ims_rx;
        ROUTER_RX router_rx;
        CONFIG_RX config_rx;
        BACKUP_RX backup_rx;
        USER_MANAGER_RX user_manager_rx;

        KEY_MANAGER_TX key_manager_tx;
        STORAGE_TX storage_tx;
        STORAGE_TX storage_writer_tx;
        STORAGE_TX cache_tx;
        OUTPUT wallet_tx;
        IMS_TX ims_tx;
        ROUTER_TX router_tx;
        CONFIG_TX config_tx;
        BACKUP_TX backup_tx;
        USER_MANAGER_TX user_manager_tx;

        /// @name Private data member
private:

        // Pending requests table. Time-out functionality is not considered yet.
        // each task that posts requests should define request id start point.
        // Most significant 4 bits will be used to divide [0, 2^32] into 16
        // ranges and each task should choose one of the ranges as domain for
        // requests ids. Lower bound of ranges are defined in protocols.hpp
        ID32 m_request_counter;

        uuid_request_map m_pending_ims_get_requests;
        std::list<ID32> m_pending_router_get_reqs;
        requests m_requests;
        kernel_ims_req_ids m_ims_req_ids;
        kernel_ims_req_ids m_stream_req_ids;
        backup_ids m_backup_ids;
        std::map<Poco::UUID,
                std::unique_ptr<livestream_info>> m_hosted_livestreams;

        /**
         * For tracking of save requests of the data units: share object,
         * master json and data pieces
         */
        std::map<ID32, save_unit> m_save_units;

        // Used for requests which command is metax::save. Contains the schedule
        // list of <ID32,UUID> pairs. First component of the pair is the parent
        // request id. Second component is the serial piece uuid of parent
        // request which should be processed.
        std::list<std::pair<ID32, UUID> > m_unhandled_save_reqs;

        /**
         * All the units for save are placed here before sending to storage and
         * router. This is done for controlling of resource (RAM, Network,
         * etc.) usage by Metax. If the resource usage limit is reached the
         * save requests are not continued after the some of the ongoing
         * requests are finished and resources are freed.
         */
        std::forward_list<ID32> m_pending_save_requests;
        std::list<ID32> m_pending_router_save_reqs;
        unsigned int m_storage_input_cnt;
        uint32_t m_router_save_input_size;
        uint32_t m_router_get_input_size;

        /**
         * Keeps UUIDs and the peers who are intrested in the update event of
         * the corresponding UUID. In case of the update happens for the UUID,
         * a message is being sent to all registered peers.
         */
        Poco::JSON::Object::Ptr m_update_listeners;

        /**
         * For serialization of m_update_listeners container
         */
        std::string m_update_listeners_path;

        /**
         * Keeps all UUIDs stored by own kernel with their keys (if encrypted)
         * Keeps all shares of own files with friend's. Frients id and aes key
         * encrypted with friend's public key is stored.
         * Keeps all the files shared by friends with me
         */
        Poco::JSON::Object::Ptr m_user_json;

        Poco::UUID m_user_json_uuid;

        /*
         * @brief see description of configuration_manager::data::storage_class
         * member
        */
        unsigned int m_storage_class;
        unsigned int m_sync_class;
        bool m_enable_livstream_hosting;
        bool m_should_block_get_request;
        bool m_valid_keys;
        unsigned short m_user_json_wait_counter;

        storage* m_get_storage;

        storage* m_save_storage;

        unsigned short max_get_fail_count;
        unsigned short max_save_fail_count;

        // Save configurations
        unsigned short m_data_copy_count;
        unsigned short min_save_pieces;
        unsigned int max_chunk_size;
        // Specifies the maximum limit of existing save requests.
        unsigned int max_storage_input_cnt;
        uint32_t max_router_save_input_size;
        uint32_t max_router_get_input_size;
        uint32_t max_get_count;

        const int recording_max_chunk_size;

        /// @name Special member functions
public:
        /// Default constructor
        kernel(storage* g, storage* s);

        /// Destructor
        virtual ~kernel();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        kernel(const kernel&);

        /// This class is not assignable
        kernel& operator=(const kernel&);

        /// This class is not copy-constructible
        kernel(const kernel&&);

        /// This class is not assignable
        kernel& operator=(const kernel&&);

}; // class leviathan::metax::kernel

#endif // LEVIATHAN_METAX_KERNEL_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

