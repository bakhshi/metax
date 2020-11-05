/**
 * @file src/metax_web_api/web_api_adapter.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax_web_api::web_api_adapter.
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

#ifndef LEVIATHAN_METAX_WEB_API_WEB_API_ADAPTER_HPP
#define LEVIATHAN_METAX_WEB_API_WEB_API_ADAPTER_HPP


// Headers from this project

// Headers from other projects
#include <platform/task.hpp>
#include <metax/protocols.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Event.h"

// Headers from standard libraries
#include <map>
#include <future>
#include <queue>

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class web_api_adapter;
                class websocket_request_handler;

                /**
                 * Definition of commands used in http requests uris
                 * none - differ uri from save/update,
                 * save_node - save/update node content,
                 * save_path - save/update path,
                 * save_data - save/update uploaded file form data
                 * save_stream - start saving the livestream
                 */
                enum URI {
                        none,
                        save_node,
                        save_path,
                        save_data,
                        save_stream
                };
        }
}

/**
 * @brief Mediator between http requests and Metax. Passes requests from
 * http request handler threads to Metax and sends responses received from Metax
 * back to the corresponding http request handler threads.
 * 
 */
class leviathan::metax_web_api::web_api_adapter
        : public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        typedef leviathan::platform::csp::input<metax::
                ims_kernel_package> INPUT;
        typedef leviathan::platform::csp::output<metax::
                ims_kernel_package> OUTPUT;

        using  UTILS = platform::utils;

        enum ims_response_type {file, content, livestream_start, livestream_content, media_streaming};

        struct ims_response {
                ims_response_type kind;
                Poco::Net::HTTPResponse::HTTPStatus st;
                std::string info;
                Poco::SharedPtr<Poco::TemporaryFile> response_file_path;
                std::string content_type;
                // seems can be unique_ptr but compilation doesn't pass, should
                // be checked why
                std::shared_ptr<std::vector<std::future<ims_response>>> stream_futures;
                std::pair<int64_t, int64_t> range;
                Poco::UInt64 content_length;

                ims_response() = default;

                ims_response(ims_response_type k,
                             Poco::Net::HTTPResponse::HTTPStatus s,
                             const std::string& i, const std::string& c = "")
                : kind(k)
                , st(s)
                , info(i)
                , response_file_path()
                , content_type(c)
                , stream_futures(nullptr)
                , range(std::make_pair(-1, -1))
                , content_length(0)
                {}
        };

        struct request_info {
                metax::ID32 id;
                std::promise<ims_response>* prom;
                /**
                 * Either stores path for save request or user id for share
                 * request.
                 */
                std::string info;
                /**
                 * Used for the share request.
                 * Stors original uuid of the shareable node.
                 */
                std::string username;
                Poco::UUID uuid;

                std::vector<std::promise<ims_response>>* m_stream_promises;
                Poco::UInt64 m_cur_stream_chunk;

                metax::command command;

                long unsigned int client_id;

                request_info(metax::ID32 i, std::promise<ims_response>* e,
                        const std::string& s = "",
                        const std::string& n = "",
                        const Poco::UUID& u = Poco::UUID::null(),
                        metax::command cmd = metax::max_command,
                        long unsigned int cid = 0)
                : id(i)
                , prom(e)
                , info(s)
                , username(n)
                , uuid(u)
                , m_stream_promises(nullptr)
                , m_cur_stream_chunk(0)
                , command(cmd)
                , client_id(cid)
                {}

                ~request_info()
                {
                        delete m_stream_promises;
                }

                request_info(const request_info&) = delete;

                /// @brief Move constructor
                request_info(request_info&& r)
                        : id(r.id)
                        , prom(r.prom)
                        , info(std::move(r.info))
                        , username(std::move(r.username))
                        , uuid(r.uuid)
                        , m_stream_promises(r.m_stream_promises)
                        , m_cur_stream_chunk(r.m_cur_stream_chunk)
                        , command(r.command)
                        , client_id(r.client_id)
                {
                        r.prom = nullptr;
                        r.m_stream_promises = nullptr;
                }
        };

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask() ;

        /**
         * @brief Returns one and only instance of this class
         *
         * Creates instance If no object is created yet.
         */
        static web_api_adapter* get_instance();

        /**
         * @brief Deletes the only instance of this class
         */
        static void delete_instance();

        /**
         * @brief returns true if the save request is from "localhost" or
         * "127.0.0.1" or non-local save is enabled from configuration file
         */
        bool is_save_allowed(const Poco::Net::HTTPServerRequest&);

        /**
         * @brief returns true if the get request is from "localhost" or
         * "127.0.0.1" or non-local get is enabled from configuration file
         */
        bool is_get_allowed(const Poco::Net::HTTPServerRequest&);

        /**
         * @brief Parse configuration from file
         *
         * @param cfg_path - the path of the configuration file
         */
        void set_configuration(const std::string& cfg_path);

        /**
         * @brief Adds newly connected websocket to the list of websocket
         * connections
         *
         * @param ws - the websocket pointer to be added
         */
        void add_websocket(websocket_request_handler* ws);

        /**
         * @brief Removes websocket from the list of websocket connections
         *
         * @param ws - the websocket pointer to be removed
         */
        void remove_websocket(websocket_request_handler* ws);

        /// @name HTTP request processing
public:
        /**
         * @brief - sends get request to Metax.
         */
        void handle_get_request(std::promise<ims_response>*,
                            const std::string& s,
                            const std::pair<int64_t, int64_t>& range,
                            bool cache,
                            unsigned long int client_id);

        /**
         * @brief - sends copy request to Metax.
         */
        void handle_copy_request(std::promise<ims_response>* pr,
                        const std::string& r, Poco::Int64 expire_date,
                        unsigned long int client_id);

        void handle_post_request(std::promise<web_api_adapter::ims_response>*,
                                const std::string& s, const std::string& c,
                                URI b, const Poco::UUID&, bool,
                                metax::ims_kernel_package::save_option,
                                Poco::Int64);

        /**
         * @brief - sends save stream request to Metax.
         */
        void handle_save_stream_request(
                        std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& s, const std::string& c,
                        const Poco::UUID&, bool enc, unsigned long client_id);

        /**
         * @brief - sends save stream request to Metax.
         */
        void handle_livestream_content(const char*, int, unsigned long);

        /**
         * @brief - sends cancel livestream request to Metax.
         */
        void cancel_livestream(unsigned long client_id);

        /**
         * @brief - sends cancel livestream get request to Metax.
         */
        void cancel_livestream_get(unsigned long client_id);

        /**
         * @brief - sends share request to Metax.
         */
        void handle_share_request(std::promise<web_api_adapter::ims_response>*,
                                     const std::string& c,
                                     const Poco::UUID&,
                                     const bool);

        /**
         * @brief - sends accept share request to Metax.
         */
        void handle_accept_share_request(
                        std::promise<web_api_adapter::ims_response>*,
                        const Poco::UUID& u,
                        const std::string& k,
                        const std::string& i);

        /**
         * @brief - sends register listener request to Metax.
         */
        void handle_register_on_update_request(std::promise<ims_response>* pr,
                                                        const Poco::UUID& u);

        /**
         * @brief - sends unregister listener request to Metax.
         */
        void handle_unregister_on_update_request(std::promise<ims_response>* pr,
                                                        const Poco::UUID& u);
        /**
         * @brief - sends delete request to Metax.
         */
        void handle_delete_request(std::promise<ims_response>* pr,
                        const Poco::UUID& u, bool keep_chunks);

        /**
         * @brief - sends send_to_peer request to Metax
         */
        void handle_send_to_peer_request(
                                    std::promise<web_api_adapter::ims_response>*,
                                    const std::string& c, const bool,
                                    const std::string&);

        /**
         * @brief - sends recording start request to Metax
         */
        void handle_recording_start_request(std::promise<ims_response>* pr,
                                                        const Poco::UUID& lu);

        /**
         * @brief - sends recording stop request to Metax
         */
        void handle_recording_stop_request(std::promise<ims_response>* pr,
                                                        const Poco::UUID& lu,
                                                        const Poco::UUID& ru);

        /**
         * @brief Moves the uuids currently in cache into storage for permanent
         * store
         */
        void handle_move_cache_to_storage_request(
                        std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Addes new peer info to Metax configuration.
         */
        void handle_add_trusted_peer_request(
                        std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& k);

        /**
         * @brief Removes peer info to Metax configuration.
         */
        void handle_remove_trusted_peer_request(
                        std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& k);

        /**
         * @brief Gets the list of trusted peers in json format
         */
        void handle_get_trusted_peer_list_request(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Addes new friend info to Metax configuration.
         *
         * The info consists of username and public key
         */
        void handle_add_friend_request(
                        std::promise<web_api_adapter::ims_response>* pr,
                        const std::string& u,
                        const std::string& k);

        /**
         * @brief Gets the list of friends in json format
         */
        void handle_get_friend_list_request(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send get_user_public_key request to metax.
         */
        void handle_get_user_public_key_request(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send get_user_keys request to metax.
         */
        void handle_get_user_keys_request(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send get_online_peers request to metax.
         */
        void handle_get_online_peers(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send get_metax_info request to metax.
         */
        void handle_get_metax_info(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send set_metax_info request to metax.
         */
        void handle_set_metax_info(
                std::promise<web_api_adapter::ims_response>* pr,
                const std::string& u);

        /**
         * @brief Send reconnect to peers request to metax.
         *
         * Metax will attempt to immediately reconnect the dropped connections.
         */
        void handle_reconnect_to_peers(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send start pairing request to metax.
         *
         * Metax will pass to pairing mode.
         */
        void handle_start_pairing(
                std::promise<web_api_adapter::ims_response>* pr,
                const std::string& timeout);

        /**
         * @brief Send cancel pairing request to metax.
         *
         * Metax will pass to pairing mode.
         */
        void handle_cancel_pairing(
                std::promise<web_api_adapter::ims_response>* pr);


        /**
         * @brief Send get_pairing_peers request to metax.
         */
        void handle_get_pairing_peers(
                std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief Send request_keys request to metax.
         */
        void handle_request_keys(
                std::promise<web_api_adapter::ims_response>* pr,
                const std::string& id, const std::string& code);

        /**
         * @brief Send regenerated user keys request to metax.
         */
        void handle_regenerate_user_keys(
                        std::promise<web_api_adapter::ims_response>* pr);

        /**
         * @brief when the client finishes streaming content to the client, IMS
         * will be informed about it. Cleans up info associated with the
         * request from the client and informs kernel about the completion of
         * streaming.
         */
        void clean_streaming_request(unsigned long client_id);

        /**
         * @brief - sends dump_user_info request to Metax.
         */
        void handle_dump_user_info(std::promise<ims_response>* pr);

        /// @name Helper functions
private:
        metax::ID32 generate_request_id ();
        // TODO shouldn't be tmp files cleaned by shared_ptr?
        void clean_tmp_files(const std::string& p);
        void handle_recording_started();
        void handle_recording_stopped();
        void handle_recording_fail();
        void send_streaming_chunk(request_info&);
        void send_complete_file(request_info&);
        void send_get_response();
        void handle_share_failed();
        void handle_accept_share_fail();
        void handle_share_accepted();
        void handle_regenerate_user_keys_succeeded();
        void handle_regenerate_user_keys_fail();

        /**
         * @brief - Handles failed share requests.
         */
        std::string construct_share_error_message();

        /**
         * @brief - Constructs error and warning messages for share requests.
         */
        void construct_send_error_message(std::string&);
        /**
         * @brief - Constructs error and warning messages for send_to requests.
         */
        void handle_send_to_peer_failed();
        /**
         * @brief - Handles failed send_to requests.
         */
        void send_save_response();
        void send_save_stream_response();
        void send_livestream_content();
        void handle_share_confirm();
        void handle_send_to_peer();
        void post_send_to_peer_request(metax::ID32 id, const std::string&,
                        const std::string&);
        void send_deliver_fail_response_to_metax();
        void deliver_notification(const std::string& msg);
        void add_message_to_queue(const std::string& msg);
        void handle_send_to_peer_confirm();
        void handle_send_share_failed();
        void handle_sync_finished();
        void handle_metax_fail();
        void send_websocket_message(const std::string& m);
        void generate_promise_future_vectors_for_streaming(
                                request_info& req, ims_response& ir);
        void handle_streaming_prepare();
        void handle_on_update_registered();
        void handle_on_update_unregistered();
        void handle_on_update_register_fail();
        void handle_on_update_unregister_fail();
        void handle_get_fail();

        /**
         * @brief - Handles negative response for metax::update_fail request.
         */
        void handle_update_fail();

        void handle_save_stream_fail();

        /**
         * @brief - Handles metax::send_to_fail, metax::share_fail and
         * metax::deliver_fail requests recieved from kernel.
         */
        void handle_metax_input();
        void process_metax_input();
        void handle_get_friend_confirm();
        void handle_get_friend_not_found();
        void handle_add_trusted_peer_confirm();
        void handle_remove_trusted_peer_confirm();
        void handle_get_trusted_peer_list_response();
        void handle_get_friend_list_response();
        void handle_add_friend_confirm();

        void handle_move_cache_to_storage_completed();
        /**
         * @brief Clean up request by id from m_get_requests
         *
         * @param id - id of request which should be cleaned up.
         */
        void clean_get_request(metax::ID32 id, unsigned long cid = 0);

        /**
         * @brief Clean up request by id from m_save_requests
         *
         * @param id - id of request which should be cleaned up.
         */
        void clean_save_request(metax::ID32 id, unsigned long cid = 0);

        /**
         * @brief Clean up request by id from m_config_requests
         *
         * @param id - id of request which should be cleaned up.
         */
        void clean_config_request(metax::ID32 id);

        metax::ID32 generate_get_request_info(std::promise<ims_response>*,
                                                        unsigned long int);
        void handle_get_livestream_fail();

        void set_exception_to_promises(
                        std::map<metax::ID32, request_info>& reqs);
        void handle_copy_succeeded_response();
        void handle_copy_failed_response();
        void send_livestream_cancel_to_client(const request_info& req);
        void handle_deleted_response();
        void handle_delete_fail();
        void handle_get_user_public_key_response();
        void handle_get_user_keys_response();
        void handle_get_user_keys_failed();
        void handle_get_online_peers_response();
        void handle_get_pairing_peers_response();
        void handle_get_generated_code();
        void handle_start_pairing_fail();
        void handle_request_keys_response();
        void handle_request_keys_fail();
        void handle_get_metax_info_resposne();
        void handle_set_metax_info_ok();
        void handle_set_metax_info_fail();
        void handle_dump_user_info_succeeded();
        void handle_add_trusted_peer_failed();
        void handle_remove_trusted_peer_failed();
        void handle_add_friend_failed();

        /// @name I/O
public:
        INPUT metax_rx;
        OUTPUT metax_tx;

        /// @name Private data
private:
        bool m_enable_non_localhost_save;
        bool m_enable_non_localhost_get;
        static web_api_adapter* s_instance;
        Poco::Mutex m_get_requests_mutex;
        Poco::Mutex m_save_requests_mutex;
        Poco::Mutex m_config_requests_mutex;
        Poco::Mutex m_metax_tx_mutex;
        std::atomic<metax::ID32> m_counter;
        std::map<metax::ID32, request_info> m_get_requests;
        std::map<metax::ID32, request_info> m_save_requests;
        std::map<metax::ID32, request_info> m_config_requests;
        std::set<websocket_request_handler*> m_wsockets;
        std::deque<std::string> m_last_messages;
        Poco::Mutex m_wsockets_mutex;
        /**
         * Associates client id with the corresponding request id. Used for get
         * request, particularly for cleaning up the streaming request.
        */
        std::map<long unsigned int, metax::ID32> m_client_to_request;
        Poco::Mutex m_client_to_request_mutex;

        /// @name Special member functions
public:
        /// Default constructor
        web_api_adapter();

        /// Destructor
        virtual ~web_api_adapter();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        web_api_adapter(const web_api_adapter&) = delete;

        /// This class is not assignable
        web_api_adapter& operator=(const web_api_adapter&) = delete;

}; // class leviathan::metax_web_api::web_api_adapter

#endif // LEVIATHAN_METAX_WEB_API_WEB_API_ADAPTER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

