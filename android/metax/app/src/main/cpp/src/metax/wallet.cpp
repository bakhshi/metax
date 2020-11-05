
/**
 * @file src/metax/wallet.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::wallet
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "wallet.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries

// Headers from standard libraries
#include <string>
#include <sstream>

void leviathan::metax::wallet::
handle_kernel_input()
{
        if (! kernel_rx.has_data()) {
                return;
        }
        std::string s((*kernel_rx).message);
        METAX_INFO("Got message from kernel: " + s);
        kernel_rx.consume();
        if(kernel_rx.is_active()) {
                (*kernel_tx).set_payload(std::string(name() + ": " + "connection established."));
                kernel_tx.commit();
        }
        kernel_tx.deactivate();
}

void leviathan::metax::wallet::
handle_router_input()
{
        if (! router_rx.has_data()) {
                return;
        }
        std::string s((*router_rx).message);
        METAX_INFO("Got message from router: " + s);
        router_rx.consume();
        if (router_rx.is_active()) {
                (*router_tx).set_payload(std::string(name() + ": " + "connection established."));
                router_tx.commit();
        }
        router_tx.deactivate();
}

void leviathan::metax::wallet::
handle_config_input()
{
        if (! config_rx.has_data()) {
                return;
        }
        std::string s((*config_rx).message);
        METAX_INFO("Got message from config: " + s);
        config_rx.consume();
        if (config_rx.is_active()) {
                (*config_tx).set_payload(std::string(name() + ": connection established."));
                config_tx.commit();
        }
        config_tx.deactivate();
}

void leviathan::metax::wallet::
runTask()
try {
        //finish();
        //return;
        //(*kernel_tx).set_payload(std::string(name() + " is active"));
        //kernel_tx.commit();
        //(*router_tx).set_payload(std::string(name() + " is active"));
        //router_tx.commit();
        //(*config_tx).set_payload(std::string(name() + " is active"));
        //config_tx.commit();
        while (true) {
                // wait to read data
                if (!wait()) {
                        break;
                }
                handle_kernel_input();
                handle_router_input();
                handle_config_input();
        }
        finish();
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch(const Poco::Exception& e) {
        METAX_FATAL("Fatal error:" + e.displayText());
        //std::terminate();
} catch(...) {
        METAX_FATAL("Fatal error");
        //std::terminate();
}

leviathan::metax::wallet::
wallet()
        : platform::csp::task("wallet", Poco::Logger::get("metax.wallet"))
        , kernel_rx(this)
        , router_rx(this)
        , config_rx(this)
        , kernel_tx(this)
        , router_tx(this)
        , config_tx(this)
{
}

leviathan::metax::wallet::
~wallet()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

