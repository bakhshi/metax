/**
 * @file src/metax_web_api/http_server_app.cpp
 *
 * @brief Implementation of the class @ref
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

// Headers from this project
#include "http_server_app.hpp"

// Headers from other projects
#include <metax/hop.hpp>
#include "web_api_adapter.hpp"
#include "http_server.hpp"

// Headers from third party libraries

// Headers from standard libraries
#include <iostream>

#define VERSION_STRING(version) #version

void leviathan::metax_web_api::http_server_app::
initialize(Application& self)
{
        loadConfiguration();
        ServerApplication::initialize(self);
}

void leviathan::metax_web_api::http_server_app::
uninitialize()
{
        ServerApplication::uninitialize();
}

void leviathan::metax_web_api::http_server_app::
defineOptions(Poco::Util::OptionSet& o)
{
        ServerApplication::defineOptions(o);
        o.addOption(
                Poco::Util::Option("help", "h", "display help information")
                .required(false)
                .repeatable(false));
        o.addOption(
                Poco::Util::Option("config", "f", "configuration file")
                .required(false)
                .repeatable(false)
                .argument("file"));
        o.addOption(
                Poco::Util::Option("token", "u", "security token")
                .required(false)
                .repeatable(false)
                .argument("string"));
        o.addOption(
                Poco::Util::Option("version", "v", "display current Metax version")
                .required(false)
                .repeatable(false));
}

void leviathan::metax_web_api::http_server_app::
handleOption(const std::string& n, const std::string& v)
{
        if ("help" == n) {
                print_usage();
                stopOptionsProcessing();
                exit(0);
        } else if ("config" == n) {
                m_cfg_path = v;
        } else if ("token" == n) {
                m_security_token = v;
        } else if ("version" == n) {
                        //std::cout << PROJECT_VERSION << std::endl; Windows can't figure out PROJECT_VERSION
                        //TODO: need to fix
                        std::cout << "1.2.7" << std::endl;
                exit(0);
        }
        ServerApplication::handleOption(n, v);
}

void leviathan::metax_web_api::http_server_app::
print_usage()
{
        std::cout << "Usage: ./bin/metax_web_api [OPTIONS]\n" << std::endl;
        std::cout << "OPTIONS" << std::endl;
        std::cout << "\t--help,-h -\t\tprint this message and exit"
                << std::endl;
        std::cout << "\t--config,-f <file> -\tconfiguration file"
                << std::endl;
        std::cout << "\t--token,-u <string> -\tsecurity token to be checked on http(s) requests"
                << std::endl;
}

int leviathan::metax_web_api::http_server_app::
main(const std::vector<std::string>& /*args*/)
try {

        namespace CSP = leviathan::platform::csp;
        namespace MWA = leviathan::metax_web_api;
        leviathan::metax::hop node(m_cfg_path);
        MWA::web_api_adapter* ims = MWA::web_api_adapter::get_instance();
        ims->set_configuration(m_cfg_path);
        poco_assert(0 != node.rx);
        poco_assert(0 != node.tx);
        CSP::connect(ims->metax_tx, *node.rx);
        CSP::connect(ims->metax_rx, *node.tx);
        CSP::manager::start();
        leviathan::metax_web_api::http_server srv(m_cfg_path, m_security_token);
        waitForTerminationRequest();
        srv.stop();
        CSP::manager::cancel_all();
        CSP::manager::join();
        return Application::EXIT_OK;
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        return Application::EXIT_SOFTWARE;
} catch (const Poco::Exception& e) {
        std::cout << "Fatal error." << std::endl;
        METAX_FATAL(e.displayText());
        return Application::EXIT_SOFTWARE;
} catch (...) {
        std::cout << "Fatal error." << std::endl;
        METAX_FATAL("Unhandled exception");
        return Application::EXIT_SOFTWARE;
}

leviathan::metax_web_api::http_server_app::
http_server_app()
        : m_cfg_path(Poco::Path::home() + ".leviathan/config.xml")
        , m_security_token("")
        , m_logger(Poco::Logger::get("metax_web_api.http_server_app"))
{}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
