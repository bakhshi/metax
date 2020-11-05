/**
 * @file src/metax_web_api/http_server.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax_web_api::http_server.
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
#include "http_server.hpp"
#include "web_request_handler_factory.hpp"
#include "checked_web_request_handler_factory.hpp"
#include "web_api_adapter.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/File.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/SecureServerSocket.h>
#include <Poco/Path.h>
#include <Poco/Util/XMLConfiguration.h>

// Headers from standard libraries
#include <fstream>

void leviathan::metax_web_api::http_server::
parse_config()
{
        try {
                Poco::AutoPtr<Poco::Util::XMLConfiguration> c(
                                new Poco::Util::XMLConfiguration(m_cfg_path));
                m_port = (unsigned short)c->getUInt("web_server_port", 7071);
                m_prvkey = Poco::Path::expand(
                                c->getString("https_certificate.private_key", ""));
                m_cert = Poco::Path::expand(
                                c->getString("https_certificate.certificate", ""));
                m_sitemap_uuid = c->getString("sitemap", "");
        } catch(Poco::RuntimeException& e) {
                METAX_INFO(e.displayText());
        }
}

std::string leviathan::metax_web_api::http_server::
name() const
{
        return "metax_web_api.http_server";
}

void leviathan::metax_web_api::http_server::
stop()
{
        poco_assert(nullptr != m_srv);
        m_srv->stopAll(true);
}

void leviathan::metax_web_api::http_server::
load_sitemap()
{
        if ("" == m_sitemap_uuid) {
                METAX_WARNING("No sitemap uuid is provided");
                return;
        }
        namespace MWA = leviathan::metax_web_api;
        MWA::web_api_adapter* adapter = MWA::web_api_adapter::get_instance();
        poco_assert(nullptr != adapter);
        std::promise<MWA::web_api_adapter::ims_response> pr;
        std::future<MWA::web_api_adapter::ims_response> fu = pr.get_future();
        try {
                adapter->handle_get_request(&pr, m_sitemap_uuid,
                        std::make_pair(-1, -1), true, (long unsigned int)this);
                MWA::web_api_adapter::ims_response ir = fu.get();
                if (Poco::Net::HTTPResponse::HTTP_OK == ir.st
                                        || ! ir.response_file_path.isNull()) {
                        std::ifstream ifs(ir.response_file_path->path());
                        namespace P = leviathan::platform;
                        m_sitemap = P::utils::parse_json<sitemap>(ifs);
                        ifs.close();
                        METAX_INFO("loaded sitemap: " + m_sitemap_uuid);
                } else {
                        METAX_ERROR("sitemap not found: " + m_sitemap_uuid);
                }
        } catch(Poco::Exception& e) {
                METAX_ERROR("Unable to load sitemap: " + e.displayText());
        } catch(...) {
                METAX_ERROR("Unable to load sitemap: " + m_sitemap_uuid);
        }
}

leviathan::metax_web_api::http_server::
http_server(const std::string& cfg_path, const std::string& token)
        : m_srv(nullptr)
        , m_cfg_path(cfg_path)
        , m_port(7071)
        , m_prvkey("")
        , m_cert("")
        , m_logger(Poco::Logger::get("metax_web_api.http_server"))
        , m_sitemap(nullptr)
        , m_sitemap_uuid("")
{
        Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams; 
        pParams->setMaxQueued(100000);
        pParams->setMaxThreads(100000);
        Poco::ThreadPool::defaultPool().addCapacity(300);
        parse_config();
        load_sitemap();
        bool is_https = "" != m_prvkey && Poco::File(m_prvkey).exists() &&
                        "" != m_cert && Poco::File(m_cert).exists();
        web_request_handler_factory* wrhf = nullptr;
        if ("" != token) {
                wrhf = new checked_web_request_handler_factory(m_sitemap, token);
        } else {
                wrhf = new web_request_handler_factory(m_sitemap);
        }
	if (is_https) {
		Poco::Net::SocketAddress sa(m_port);
		Poco::Net::SecureServerSocket svs(sa, 64,
			new Poco::Net::Context( Poco::Net::Context::SERVER_USE,
				m_prvkey, m_cert, "",
				Poco::Net::Context::VERIFY_NONE, 9, false,
				"ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));
                m_srv = new Poco::Net::HTTPServer(wrhf, svs, pParams);
                METAX_INFO("Starting https server");
	} else {
                Poco::Net::ServerSocket svs;
                // Calling bind function for disabling SO_REUSEPORT option for
                // socket.
                svs.bind(m_port, true, false);
                svs.listen();
                m_srv = new Poco::Net::HTTPServer(wrhf, svs, pParams);
                METAX_INFO("Starting http (non-secure) server");
	}
        m_srv->start();
}

leviathan::metax_web_api::http_server::
~http_server()
{
        // Moved into http_server_app
        // m_srv->stop();
        delete m_srv;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:expandtab
