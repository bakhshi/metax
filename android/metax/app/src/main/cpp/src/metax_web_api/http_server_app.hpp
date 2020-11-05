/**
 * @file src/metax_web_api/http_server_app.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax_web_api::http_server_app.
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

#ifndef LEVIATHAN_METAX_WEB_API_HTTP_SERVER_APP_HPP
#define LEVIATHAN_METAX_WEB_API_HTTP_SERVER_APP_HPP


// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Util/ServerApplication.h>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax_web_api {
                class http_server_app;
        }
}

/**
 * @brief Subclass of Poco::Util::ServerApplication. Instantiates Metax and http
 * server, connects Metax external input/output ports to web api adapter.
 * Launches all the threads and waits till termination signal. After termination
 * stops all running threads before exiting the application. Also does the
 * handling of command line options.
 * 
 */
class leviathan::metax_web_api::http_server_app
        :  public Poco::Util::ServerApplication
{
        // @name Virtual functions needed to override
protected:
        void initialize(Application& self);

        void uninitialize();

        void defineOptions(Poco::Util::OptionSet& options);

        void handleOption(const std::string&,
                                const std::string&);

        int main(const std::vector<std::string>& args);

        /// @name Helper functions
private:
        void print_usage();

        /// @name Private Data
private:
        std::string m_cfg_path;
        std::string m_security_token;
        Poco::Logger& m_logger;

        /// @name Special member functions
public:
        /// Default constructor
        http_server_app();

        /// Destructor
        virtual ~http_server_app() {};

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        http_server_app(const http_server_app&) = delete;

        /// This class is not assignable
        http_server_app& operator=(const http_server_app&) = delete;

}; // class leviathan::metax_web_api::http_server_app

#endif // LEVIATHAN_METAX_WEB_API_HTTP_SERVER_APP_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

