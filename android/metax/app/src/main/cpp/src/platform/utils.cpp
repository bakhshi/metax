/**
 * @file src/platform/utils.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::platform::utils
 *
 *
 * COPYWRITE_TODO
 *
 */


// Headers from this project
#include "utils.hpp"

// Headers from other projects
#include <Poco/Base64Encoder.h>
#include <Poco/Base64Decoder.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>

// Headers from third party libraries

// Headers from standard libraries
#include <fstream>


std::string leviathan::platform::utils::tmp_dir = Poco::Path::temp();

/**
 * @brief trim from start (in place)
 */
void leviathan::platform::utils::
ltrim(std::string &s)
{
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                                return !std::isspace(ch);
                                }));
}

/**
 * @brief trim from end (in place)
 */
void leviathan::platform::utils::
rtrim(std::string &s)
{
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                                return !std::isspace(ch);
                                }).base(), s.end());
}

/**
 * @brief trim from both ends (in place)
 */
void leviathan::platform::utils::
trim(std::string &s)
{
        ltrim(s);
        rtrim(s);
}

std::string leviathan::platform::utils::
base64_encode(const std::string& str)
{
        std::stringstream ss;
        Poco::Base64Encoder b(ss);
        b.write(str.c_str(), str.size());
        b.close();
        return ss.str();
}

std::string leviathan::platform::utils::
base64_encode(default_package pkg)
{
        poco_assert(nullptr != pkg.message.get());
        std::stringstream ss;
        Poco::Base64Encoder b(ss);
        b.write(pkg.message.get(), pkg.size);
        b.close();
        return ss.str();
}

std::string leviathan::platform::utils::
base64_decode(const std::string& str)
{
        std::istringstream ss(str);
        Poco::Base64Decoder b(ss);
        std::string dec;
        Poco::StreamCopier::copyToString(b, dec);
        return dec;
}

std::string leviathan::platform::utils::
base64_encode(const std::vector<unsigned char>& vec)
{
        std::stringstream ss;
        Poco::Base64Encoder b(ss);
        std::string str(vec.begin(), vec.end());
        b.write(str.c_str(), str.size());
        b.close();
        return ss.str();
}

std::string leviathan::platform::utils::
read_file_content(const std::string& p, bool trim)
{
        std::string c = "";
        std::ifstream ifs(p);
        if (ifs.is_open()) {
                ifs.seekg(0, std::ios::end);
                int32_t length = (uint32_t)ifs.tellg();
                if (0 != length) {
                        ifs.seekg(0, std::ios::beg);
                        c.resize(length);
                        ifs.read(&c[0], length);
                        if (trim) {
                                platform::utils::trim(c);
                        }
                }
                ifs.close();
        }
        return c;
}
