/**
 * @file src/metax_web_api/http_server.hpp
 *
 * @brief Definition of the class @ref
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

#ifndef LEVIATHAN_METAX_WEB_API_HTTP_SERVER_HPP
#define LEVIATHAN_METAX_WEB_API_HTTP_SERVER_HPP


// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Net/HTTPServer.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>

// Headers from standard libraries
#include <string>

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class http_server;
        }
}

/**
 * @brief Instantiates HTTP or HTTPS server.
 * 
 */
class leviathan::metax_web_api::http_server
{
        /// Data member
private:
        Poco::Net::HTTPServer* m_srv;
        std::string m_cfg_path;
        unsigned short m_port;
        std::string m_prvkey;
        std::string m_cert;
        Poco::Logger& m_logger;

        /// Helper functions
private:
        void parse_config();
        std::string name() const;

        /// @name Sitemap for web-server routing
public:
        using sitemap = Poco::JSON::Object::Ptr;
        sitemap get_sitemap() { return m_sitemap; }
        void load_sitemap();
        void stop();
private:
        sitemap m_sitemap;
        std::string m_sitemap_uuid;

        /// @name Special member functions
public:
        /// Constructor
        http_server(const std::string& cfg_path, const std::string& token = "");

        /// Destructor
        virtual ~http_server();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        http_server(const http_server&) = delete;

        /// This class is not assignable
        http_server& operator=(const http_server&) = delete;

}; // class leviathan::metax_web_api::http_server

#endif // LEVIATHAN_METAX_WEB_API_HTTP_SERVER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

