/**
 * @file src/platform/default_package.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::platform::default_package
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "default_package.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/ByteOrder.h>

// Headers from standard libraries

void
leviathan::platform::default_package::
resize(uint32_t s)
{
        size = s;
}

void
leviathan::platform::default_package::
set_payload(const char *const p)
{
        poco_assert(0 != size);
        char* tmp = new char[size + 1];
        std::memcpy(tmp, p, size);
        tmp[size] = '\0';
        message = tmp;
}

void
leviathan::platform::default_package::
set_payload(char * p)
{
        poco_assert(0 != size);
        message = p;
}

void
leviathan::platform::default_package::
set_payload(const std::string& p)
{
        resize((uint32_t)p.size());
        poco_assert(p.size() == size);
        char* tmp = new char[size + 1];
        std::memcpy(tmp, p.c_str(), size);
        tmp[size] = '\0';
        message = tmp;
}

void
leviathan::platform::default_package::
set_payload(const Poco::JSON::Object::Ptr obj)
{
        poco_assert(! obj.isNull());
        std::stringstream ss;
        obj->stringify(ss);
        set_payload(ss.str());
}

void
leviathan::platform::default_package::
set_payload(const default_package& p)
{
        default_package::operator=(p);
}

void
leviathan::platform::default_package::
reset()
{
        message = nullptr;
        size = 0;
        status = false;
}

void
leviathan::platform::default_package::
extract_payload(const default_package& pkg, unsigned long int offset)
{
        reset();
        if (offset + sizeof(pkg.size) > pkg.size) {
                throw Poco::Exception("ill-formed package");
        }
        const char* src = pkg.message.get();
        uint32_t n_size;
        std::memcpy((char*)(&n_size), src + offset, sizeof(pkg.size));
        n_size = Poco::ByteOrder::fromNetwork(n_size);
        flags = (unsigned char)((n_size >> 24) & 0x000000FF);
        size = n_size & 0x00FFFFFF;
        offset += sizeof(size);
        if (0 != size) {
                if (offset + size > pkg.size) {
                        throw Poco::Exception(
                                        "payload size exceeds package size");
                }
                message = new char[size + 1];
                char* dest = message.get();
                std::memcpy(dest, src + offset, size);
                dest[size] = '\0';
        }
}

leviathan::platform::default_package::
default_package()
        : size(0)
        , version(0)
        , flags(NONE)
        , message(0)
        , status(false)
{}

leviathan::platform::default_package::
default_package(const std::string& s)
        : version(0)
        , flags(NONE)
        , status(false)
{
        set_payload(s);
}

leviathan::platform::default_package::
~default_package()
{
        size = 0;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

