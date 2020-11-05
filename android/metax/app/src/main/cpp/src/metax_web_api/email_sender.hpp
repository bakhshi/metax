/**
 * @file src/metax_web_api/email_sender.hpp
 *
 * @brief Definition of the classes @ref
 * leviathan::metax_web_api::email_sender,
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

#ifndef LEVIATHAN_METAX_WEB_API_EMAIL_SENDER_HPP
#define LEVIATHAN_METAX_WEB_API_EMAIL_SENDER_HPP


// Headers from this project

// Headers from other projects
//#include <metax/protocols.hpp>

// Headers from third part libraries
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Logger.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class email_sender;
        }
}

/**
 * @brief Handles sendemail http request.
 * 
 */
class leviathan::metax_web_api::email_sender
        : public Poco::Net::HTTPRequestHandler
{

        /// @name Public interface
public:
        void send_email();

        /**
         * @brief Processes http sendemail request.
         *
         */
        void handleRequest(Poco::Net::HTTPServerRequest& request, 
                        Poco::Net::HTTPServerResponse& response);

        void set_to(const std::string& t);

        void set_from(const std::string& f);

        void set_password(const std::string& p);

        void set_subject(const std::string& s);

        void set_content(const std::string& c);

        void set_server(const std::string& s);

        void set_port(const std::string& s);

        /// @name Helper functions
private:
        void send_email_with_login();
        void parse_email(Poco::Net::HTTPServerRequest&);
        void parse_from(const std::string&);
        void add_recipients(const std::string&, const Poco::Net::MailRecipient::RecipientType&);
        void compose_email();
        void set_response_header(Poco::Net::HTTPServerResponse&);
        bool is_valid_address(const std::string&);
        bool is_valid_password(const std::string&);
	std::string name() const;

        /// @name Private members
private:
        std::string m_message;
        std::string m_to;
        std::string m_password;
        std::string m_subject;
        std::string m_from_email;
        std::string m_from;
        std::string m_content;
        std::string m_smpt_server;
        short unsigned int m_smpt_port;
        Poco::Net::MailMessage::Recipients m_recipients;
        Poco::Logger& m_logger;

        /// @name Special member functions
public:
        /// Default constructor
        email_sender();

        /// Destructor
        ~email_sender();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        email_sender(const email_sender&) = delete;

        /// This class is not assignable
        email_sender& operator=(const email_sender&) = delete;

}; // class leviathan::metax_web_api::email_sender


#endif // LEVIATHAN_METAX_WEB_API_EMAIL_SENDER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

