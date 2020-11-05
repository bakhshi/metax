/**
 * @file src/platform/default_package.hpp
 *
 * @brief Defines Metax communication protocol.
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_DEFAULT_PACKAGE_HPP
#define LEVIATHAN_PLATFORM_DEFAULT_PACKAGE_HPP

#define UNDEFINED 0x00

// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/SharedPtr.h>
#include <Poco/JSON/Object.h>

// Headers from standard libraries
#include <cstring>

// Forward declarations
namespace leviathan {
        namespace platform {
                struct default_package;
        }
}

/**
 * @brief Definition of the default package which can be as base for all other
 * protocols.
 *
 * Assumed that max message size will not exceed 10 * 2^20 bytes. So overall size
 * of the package will not exceed 24 bits.
 *
 * When package is sent via link to remote peers the following protocol is used
 *      - (frags << 24 && size) is sent at first
 *      - message is sent second
 *
 * Each package should take care of setting flags member if there is a special
 * handling for received data.
 * For example if flags is set to LINK_PACKAGE it
 * means that received package is Link layer specific and it should not be
 * passed to router.
 * Packages sent from Link to link during negotiation should be marked by
 * LINK_PACKAGE flag in order to avoid passing them to upper layers.
 */
struct leviathan::platform::default_package
{
        /// @name Public type definitions
public:
        typedef Poco::SharedPtr<char, Poco::ReferenceCounter, 
                                Poco::ReleaseArrayPolicy<char> > payload;

        /// @name Public interface
public:
        /**
         * @brief - Set the package size
         *
         * @param s - new size of the package
         */
        void resize(uint32_t s);

        /**
         * @brief - Make package from the provided string literal
         *
         * @param p - source string to construct package
         */
        void set_payload(const char *const p);

        /**
         * @brief - Make package from the provided string literal
         * Takes ownership of the pointer
         * One should not manually delete pointer after assignment.
         * Make sure that package is resized accordingly before/after assignment
         *
         * @param p - source string to construct package
         */
        void set_payload(char * p);

        /**
         * @brief - Make package from the provided std::string
         *
         * @param p - source string to construct package
         */
        void set_payload(const std::string& p);

        /**
         * @brief - Make package from the provided JSON object. JSON is
         * stringified before setting to package
         *
         * @param obj - source JSON object to construct package
         */
        void set_payload(const Poco::JSON::Object::Ptr obj);

        /**
         * @brief - Make package from another package
         *
         * @param p - source package
         */
        void set_payload(const default_package& p);

        /**
         * @brief - Deletes package payload, sets size to 0
         *
         */
        void reset();

        /**
         * @brief - Extracts payload coming after header information
         *
         * @param pkg - source package from which payload is extracted
         * @param offset - offset from which payload starts
         */
        void extract_payload(const default_package& pkg,
                        unsigned long int offset = 0);

        /// @name Public members
public:
        enum FLAGS : unsigned char {
                NONE = 0x00,
                PING = 0x01,
                PONG = 0x02,
                LINK_MSG = 0x03
        };
        uint32_t size;
        uint32_t version;
        unsigned char flags;
        payload message;
        bool status;

        /// @name Special member functions
public:
        /// @brief Default Constructor
        default_package();

        /*
         * @brief Constructor. Constructs package from a string.
         *
         * @param s - source string
         */
        default_package(const std::string& s);

        /// Destructor
        virtual ~default_package();

}; // class leviathan::platform::default_package

#endif // LEVIATHAN_PLATFORM_DEFAULT_PACKAGE_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

