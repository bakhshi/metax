/**
 * @file src/metax/hop.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::hop
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_HOP_HPP
#define LEVIATHAN_METAX_HOP_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects

// Headers from third party libraries
#include <platform/fifo.hpp>

// Headers from standard libraries

// Forward declarations
namespace leviathan {
        namespace metax {
                struct hop;
                struct configuration_manager;
                struct kernel;
                struct router;
                struct link;
                struct wallet;
                struct storage;
                struct key_manager;
                struct backup;
                struct user_manager;
        }
}

/**
 * @brief Represents one instance of Metax - a Metax node. Instantiates all the
 * tasks, creates connections between them.
 */
struct leviathan::metax::hop
{
        /// Types used in the class.
public:
        /// Public type for messages
        typedef leviathan::platform::csp::input<ims_kernel_package> INPUT;
        typedef leviathan::platform::csp::output<ims_kernel_package> OUTPUT;

        /// @name Special member functions
public:
        /**
         * @brief Constructor.
         *
         * @param cfg_file - path to the configuration file.
         * @param base_path - there are paths in config file, like storage
         * path, public/private key files, etc. If those paths are not absolute
         * paths then base_path as appended at the beginnings of those paths.
         */
    hop(const std::string& cfg_file, const std::string& base_path = "");

        /// Destructor
        virtual ~hop();

        /// @name Private helper member functions
        void connect(leviathan::metax::configuration_manager* c,
                     leviathan::metax::kernel* k,
                     leviathan::metax::router* r,
                     leviathan::metax::link* l,
                     leviathan::metax::wallet* w,
                     leviathan::metax::storage* s,
                     leviathan::metax::storage* sw,
                     leviathan::metax::storage* sc,
                     leviathan::metax::key_manager* key,
                     leviathan::metax::backup* b,
                     leviathan::metax::user_manager* u);

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        hop(const hop&);

        /// This class is not assignable
        hop& operator=(const hop&);
        
        /// This class is not copy-constructible
        hop(const hop&&);

        /// This class is not assignable
        hop& operator=(const hop&&);

public:
        INPUT* rx;
        OUTPUT* tx;

}; // class leviathan::metax::hop

#endif // LEVIATHAN_METAX_HOP_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

