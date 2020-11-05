/**
 * @file src/metax_web_api/db_request_handler.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::db_request_handler,
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

#ifndef LEVIATHAN_METAX_WEB_API_DB_REQUEST_HANDLER_HPP
#define LEVIATHAN_METAX_WEB_API_DB_REQUEST_HANDLER_HPP


// Headers from this project
#include "web_api_adapter.hpp"

// Headers from other projects
#include <metax/protocols.hpp>

// Headers from third party libraries
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/PartHandler.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Event.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class db_request_handler;
                class upload_handler;
        }
}

/**
 * @brief Handles http requests from web client related to database operations.
 * 
 */
class leviathan::metax_web_api::db_request_handler
        : public Poco::Net::HTTPRequestHandler
{

        /**
         * Const members associated with web request url
         */
        static const std::string URI_DB;
        static const std::string URI_DB_GET;
        static const std::string URI_DB_SAVE;
        static const std::string URI_DB_COPY;
        static const std::string URI_DB_SAVE_DATA;
        static const std::string URI_DB_SAVE_PATH;
        static const std::string URI_DB_SAVE_NODE;
        static const std::string URI_DB_SAVE_STREAM;
        static const std::string URI_DB_SHARE;
        static const std::string URI_DB_ACCEPT_SHARE;
        static const std::string URI_DB_SEND_TO;
        static const std::string URI_DB_MV_CACHE_TO_STORAGE;
        static const std::string URI_DB_REGISTER_LISTENER;
        static const std::string URI_DB_UNREGISTER_LISTENER;
        static const std::string URI_DB_DELETE;
        static const std::string URI_DB_RECORDING;
        static const std::string URI_DB_RECORDING_START;
        static const std::string URI_DB_RECORDING_STOP;
        static const std::string URI_DB_DUMP_USER_INFO;

        /// @name Public interface
public:
        /**
         * @brief Processes http requests related to Metax database operations.
         */
        void handleRequest(Poco::Net::HTTPServerRequest& request, 
                        Poco::Net::HTTPServerResponse& response) override;

        /**
         * @brief - Handles web post requets (save and upload)
         * @param request - Request object with all necessary information
         * @param u - Request URI
         * @param s - Data content or path to file to be saved
         * @param uuid - UUID in case of update request, otherwise not used
         * @param l - specifies whther data should be save also locally
         * @param e - Expire date of data, time since Epoch
         */
        bool handle_web_save_request(Poco::Net::HTTPServerRequest& request,
                        URI u, std::string& s, Poco::UUID& uuid,
                        metax::ims_kernel_package::save_option& l,
                        Poco::Int64& e);

        void parse_expire_date(Poco::Net::HTMLForm& f, Poco::Int64& e);

        //Gets http request type
        URI get_request_type(const std::string& u);

        /// @name Helper Functions
private:
        leviathan::metax::ims_kernel_package::save_option
                get_save_option(const Poco::Net::HTMLForm& f) const;

        void send_get_requests(const std::string&,
                leviathan::platform::default_package&);

        void set_response_headers(Poco::Net::HTTPServerResponse*);

        void send_error_response(Poco::Net::HTTPServerResponse*,
                                        const std::string&);

        void wait_for_response(std::future<web_api_adapter::ims_response>&,
                Poco::Net::HTTPServerResponse&);

        void set_streaming_headers(Poco::Net::HTTPServerResponse&,
                                        const web_api_adapter::ims_response&);

        void start_media_streaming(Poco::Net::HTTPServerResponse&,
                        web_api_adapter::ims_response&);
        void send_file(Poco::Net::HTTPServerResponse& res,
                        web_api_adapter::ims_response& ir);

        void read_livestream_from_socket(web_api_adapter::ims_response& ir,
                        Poco::Net::HTTPServerResponse& res);
        void read_from_socket(std::future<web_api_adapter::ims_response>& e,
                web_api_adapter* ims);
        void process_save_stream_request(
                        std::promise<web_api_adapter::ims_response>& pr,
                        const std::string& s, const std::string& c,
                        const Poco::UUID& uuid, bool enc,
                        unsigned long client_id);
        void start_sending_livestream(web_api_adapter::ims_response& ir,
                                        Poco::Net::HTTPServerResponse& res);

        std::pair<int64_t, int64_t> parse_range(const std::string&);

        void handle_get_request(Poco::Net::HTTPServerRequest&,
                std::promise<web_api_adapter::ims_response>&);

        void handle_copy_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_save_request(Poco::Net::HTTPServerRequest&,
                                const std::string&,
                                std::promise<web_api_adapter::ims_response>&);

        void handle_share_request(Poco::Net::HTTPServerRequest&,
                        std::promise<web_api_adapter::ims_response>&);

        void handle_accept_share_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_send_to_peer_request(Poco::Net::HTTPServerRequest&,
                        std::promise<web_api_adapter::ims_response>&);

        void handle_register_on_update(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_unregister_on_update(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_delete_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_recording_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr,
                const std::string& req_uri);

        void handle_recording_start_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_recording_stop_request(Poco::Net::HTTPServerRequest& req,
                std::promise<web_api_adapter::ims_response>& pr);

        void handle_dump_user_info(
                        std::promise<web_api_adapter::ims_response>& pr);

        std::string name() const;

        /// @name Data members
private:
        Poco::Net::StreamSocket m_livestream_socket;
        Poco::Logger& m_logger;

        /// @name Special member functions
public:
        /// Default constructor
        db_request_handler()
                : m_logger(Poco::Logger::get("metax_web_api.db_request_handler"))
        {
        }

        /// Destructor
        virtual ~db_request_handler() {};

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        db_request_handler(const db_request_handler&) = delete;

        /// This class is not assignable
        db_request_handler& operator=(const db_request_handler&) = delete;

}; // class leviathan::metax_web_api::db_request_handler


/**
 * @brief Handles file uploads via HTML forms
 */
class leviathan::metax_web_api::upload_handler:
        public Poco::Net::PartHandler
{
        /// @name public API
public:
        /**
         * @brief Processes uploaded HTML forms via http POST request.
         *
         * @param header - message header
         * @param istr - POST body in HTML form
         */
        virtual void handlePart(const Poco::Net::MessageHeader& header, std::istream& istr);

        /**
         * Gets uploaded data path, it can be file absolute path in local machine
         * or the temporary created data path in local machine
         */
        const std::string& get_upload_file_path();

        /// @name Private data members
private:
        // Contains temporary created file path for uploaded data
        std::string m_upload_path;

        /// @name Special member functions
public:
        /**
         * @brief Default constructor
         */
        upload_handler();

}; // class leviathan::metax_web_api::db_request_handler

#endif // LEVIATHAN_METAX_WEB_API_DB_REQUEST_HANDLER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

