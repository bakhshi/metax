/**
 * @file src/metax/wallet.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::wallet
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_WALLET_HPP
#define LEVIATHAN_METAX_WALLET_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries

// Headers from standard libraries
#include <unordered_map>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct wallet;
        }
}

/**
 * @brief Implements accounting of cryptocurrency (luma) transactions.
 * 
 */
struct leviathan::metax::wallet:
        public leviathan::platform::csp::task
{
        /// Types used in the class.
public:
        /// Public type for messages
        typedef leviathan::platform::csp::input<platform::default_package> INPUT;
        typedef leviathan::platform::csp::output<platform::default_package> OUTPUT;

        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        virtual void runTask() ;

        /// @name Helper functions
private:
        void handle_kernel_input();
        void handle_router_input();
        void handle_config_input();

        /// @name Public members
public:
        INPUT kernel_rx;
        INPUT router_rx;
        INPUT config_rx;

        OUTPUT kernel_tx;
        OUTPUT router_tx;
        OUTPUT config_tx;

        /// @name Special member functions
public:
        /// Default constructor
        wallet();

        /// Destructor
        virtual ~wallet();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        wallet(const wallet&);

        /// This class is not assignable
        wallet& operator=(const wallet&);
        
        /// This class is not copy-constructible
        wallet(const wallet&&);

        /// This class is not assignable
        wallet& operator=(const wallet&&);

}; // class leviathan::metax::wallet

#endif // LEVIATHAN_METAX_WALLET_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

