/**
 * @file src/metax_web_api/email_sender.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::email_sender.
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
#include "email_sender.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/Process.h>
#include <Poco/String.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/MailRecipient.h>
#include <Poco/Net/SecureSMTPClientSession.h>
#include <Poco/Net/StringPartSource.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/KeyConsoleHandler.h>
#include <Poco/Net/ConsoleCertificateHandler.h>
#include <Poco/StringTokenizer.h>

// Headers from standard libraries
#include <iostream>
#include <fstream>
#include <regex>

class SSLInitializer
{
public:
	SSLInitializer()
	{
		Poco::Net::initializeSSL();
	}

	~SSLInitializer()
	{
		Poco::Net::uninitializeSSL();
	}
};

void leviathan::metax_web_api::email_sender::
handleRequest(Poco::Net::HTTPServerRequest& req,
                Poco::Net::HTTPServerResponse& res)
try {
        ///if ("POST" != req.getMethod()) {
        ///        std::string m = "POST request expected\n";
        ///        res.sendBuffer(m.c_str(), m.size());
        ///}
	SSLInitializer sslInitializer;
        METAX_INFO("Received email send request");
        parse_email(req);
        send_email_with_login();
        //send_email();
        set_response_header(res);
        std::string m = R"({"success":1})";
        res.sendBuffer(m.c_str(), m.size());
        METAX_INFO("Email sent successfully.");
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch (const Poco::Exception& e) {
        std::string m = R"({"success":0", "err": ")" + e.message() + R"("})";
        METAX_ERROR("Error: " + e.displayText());
        res.sendBuffer(m.c_str(), m.size());
} catch (const std::exception& e) {
        std::string w = e.what();
        std::string m = R"({"success":0", "err": ")" + w + R"("})";
        METAX_ERROR("Error: " + w);
        res.sendBuffer(m.c_str(), m.size());
} catch (...) {
        std::string m = R"({"success":0", "err": "Unknown reason"})";
        METAX_ERROR("Error: in send email request");
        res.sendBuffer(m.c_str(), m.size());
}

void leviathan::metax_web_api::email_sender::
set_response_header(Poco::Net::HTTPServerResponse& r)
{
        r.set("Cache-Control", "no-cache, no-store, must-revalidate");
        r.set("Access-Control-Allow-Origin", "*");
        r.set("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
        r.set("Access-Control-Max-Age", "1000");
        r.set("Access-Control-Allow-Headers", "Content-Type");
        r.set("Access-Control-Allow-Headers", "Metax-Content-Type");
}

void leviathan::metax_web_api::email_sender::
parse_email(Poco::Net::HTTPServerRequest& req)
{
        Poco::Net::HTMLForm f(req, req.stream());
        if (f.has("to")) {
                add_recipients(f.get("to"), Poco::Net::MailRecipient::PRIMARY_RECIPIENT);
        }
        if (f.has("cc")) {
                add_recipients(f.get("cc"), Poco::Net::MailRecipient::CC_RECIPIENT);
        }
        if (f.has("bcc")) {
                add_recipients(f.get("bcc"), Poco::Net::MailRecipient::BCC_RECIPIENT);
        }
        if (f.has("password")) {
                m_password = f.get("password");
        }
        if (! f.has("from")) {
                throw Poco::Exception("No from address is provided");
        }
        m_from = f.get("from");
        parse_from(m_from);
        if (! f.has("message")) {
                throw Poco::Exception("No email body is provided\n");
        }
        m_content = f.get("message");
        if (f.has("subject")) {
                set_subject(f.get("subject"));
        }
        if (! f.has("server")) {
                throw Poco::Exception("No SMTP server is provided\n");
        }
        m_smpt_server = f.get("server");
        if (f.has("port")) {
                set_port(f.get("port"));
        }
}

void leviathan::metax_web_api::email_sender::
add_recipients(const std::string& recipients, const Poco::Net::MailRecipient::RecipientType& type)
{
        Poco::Net::MailRecipient m;
        Poco::StringTokenizer st(recipients, ",");
        for (auto it : st) {
                Poco::trimInPlace(it);
                if (! it.empty() ) {
                        m.setAddress(it);
                        m.setType(type);
                        m_recipients.push_back(m);
                }
        }
}

void leviathan::metax_web_api::email_sender::
parse_from(const std::string& f)
{
        auto pos = f.find('<');
        if (std::string::npos == pos) {
                m_from_email = f;
                return;
        }
        std::size_t pos1 = f.find('>');
        if (std::string::npos == pos1) {
                m_from_email = f;
                return;
        }
        std::size_t count = pos1 - pos - 1;
        m_from_email = f.substr(++pos, count);
}

void leviathan::metax_web_api::email_sender::
set_to(const std::string& t)
{
        m_to = t;
}

void leviathan::metax_web_api::email_sender::
set_from(const std::string& f)
{
        m_from = f;
}

void leviathan::metax_web_api::email_sender::
set_subject(const std::string& s)
{
        m_subject = s;
}

void leviathan::metax_web_api::email_sender::
set_content(const std::string& c)
{
        m_content = c;
}

void leviathan::metax_web_api::email_sender::
set_server(const std::string& s)
{
        m_smpt_server = s;
}

void leviathan::metax_web_api::email_sender::
set_port(const std::string& s)
{
        std::stringstream parser(s);
        if (! (parser >> m_smpt_port) ) {
                throw Poco::Exception("The provided port is not valid\n");
        }
}

void leviathan::metax_web_api::email_sender::
compose_email()
{
        m_message = "To: " + m_to + '\n';
        m_message += "Subject: " + m_subject + '\n';
        m_message += "From: " + m_from + '\n';
        m_message += "Content-Type: text/plain; charset=\"utf-8\"\n";
        m_message += m_content  + '\n';
        m_message += '\n';
}

bool leviathan::metax_web_api::email_sender::
is_valid_address(const std::string& a)
{
        std::string r = R"(([[:w:]]|.|_|-)+@[[:w:]]+\.[[:w:]]+)";
        std::regex p(r);
        return regex_match(a, p);
}

void leviathan::metax_web_api::email_sender::
send_email_with_login()
{
        Poco::Net::MailMessage message;
        message.setSender(m_from);
        message.setRecipients(m_recipients);

        message.setSubject(m_subject);
        message.setContent(m_content);
        message.setContentType("text/html");

        Poco::Net::SecureSMTPClientSession session(
                        m_smpt_server, m_smpt_port);
        session.login();
        if (! m_password.empty()) {
                session.startTLS(Poco::Net::SSLManager::instance().defaultClientContext());
                session.login(Poco::Net::SMTPClientSession::AUTH_LOGIN, m_from_email, m_password);
        }
        session.sendMessage(message);
        session.close();
}

void leviathan::metax_web_api::email_sender::
send_email()
{
        compose_email();
        std::string cmd("/usr/sbin/sendmail");
        std::vector<std::string> args;
        args.push_back("-t");
        Poco::Pipe inPipe;
        Poco::ProcessHandle ph = Poco::Process::launch(cmd, args, &inPipe, 0, 0);
        Poco::PipeOutputStream ostr(inPipe);
        ostr << m_message;
        ostr.close();
        int rc = ph.wait();
        if (0 != rc) {
                throw Poco::Exception("Couldn't send email");
        }
}

std::string leviathan::metax_web_api::email_sender::
name() const
{
	return "metax_web_api.email_sender";
}

leviathan::metax_web_api::email_sender::
email_sender()
        : m_message("")
        , m_to("")
        , m_password("")
        , m_subject("From Metax")
        , m_from_email("")
        , m_from("")
        , m_content("")
        , m_smpt_server("")
        , m_smpt_port(587)
        , m_recipients()
        , m_logger(Poco::Logger::get("metax_web_api.email_sender"))
{}

leviathan::metax_web_api::email_sender::
~email_sender()
{}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

