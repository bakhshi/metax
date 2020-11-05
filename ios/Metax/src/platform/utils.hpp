/**
 * @file src/platform/utils.hpp
 *
 * @brief Provides vairous utility functions
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_UTILS_HPP
#define LEVIATHAN_PLATFORM_UTILS_HPP

// Headers from this project
#include "default_package.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/JSON/Parser.h>

// Headers from standard libraries
#include <string>
#include <sstream>

#ifdef ANDROID
#include <android/log.h>
#endif

// Forward declarations

#ifdef ANDROID
        #define METAX_FATAL(arg) \
               __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_error(m_logger, arg)

        #define METAX_CRITICAL(arg) \
               __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_error(m_logger, arg)

        #define METAX_ERROR(arg) \
               __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_error(m_logger, arg)

        #define METAX_WARNING(arg) \
               __android_log_write(ANDROID_LOG_WARN, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_warning(m_logger, arg);

        #define METAX_NOTICE(arg) \
               __android_log_write(ANDROID_LOG_INFO, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_notice(m_logger, arg);

        #define METAX_INFO(arg) \
               __android_log_write(ANDROID_LOG_INFO, "LEVIATHAN", (name() + ": " + arg).c_str());\
                poco_information(m_logger, arg);

        #define METAX_DEBUG(arg) \
               __android_log_write(ANDROID_LOG_DEBUG, "LEVIATHAN", (name() + ": " + arg).c_str());\
                if (m_logger.debug()) { \
                        m_logger.debug(arg); \
                }

        #define METAX_TRACE(arg) \
               __android_log_write(ANDROID_LOG_VERBOSE, "LEVIATHAN", (name() + ": " + arg).c_str());\
                if (m_logger.trace()) { \
                        m_logger.trace(arg); \
                }

#else
        #define METAX_FATAL(arg) \
                poco_fatal(m_logger, arg);

        #define METAX_CRITICAL(arg) \
                poco_critical(m_logger, arg);

        #define METAX_ERROR(arg) \
                poco_error(m_logger, arg);

        #define METAX_WARNING(arg) \
                poco_warning(m_logger, arg);

        #define METAX_NOTICE(arg) \
                poco_notice(m_logger, arg);

        #define METAX_INFO(arg) \
                poco_information(m_logger, arg);

        // poco_debug and poco_trace macroes are requiring _DEBUG macro
        // to be defined.
        #define METAX_DEBUG(arg) \
                if (m_logger.debug()) { \
                        m_logger.debug(arg); \
                }

        #define METAX_TRACE(arg) \
                if (m_logger.trace()) { \
                        m_logger.trace(arg); \
                }

#endif

// Forward declarations
namespace leviathan {
        namespace platform {
                struct utils;
        }
}

/**
 * @brief Provides various utility functions
 */
struct leviathan::platform::utils {
public:
        static std::string tmp_dir;

        /**
         * $brief Convert argument to std::string
         * There is a std::to_string in c++11, but it is absent in gcc 4.7.3
         * which is used by KreaTV SDK
         *
         * @param n - source value
         *
         * @return std::string representation of the provided value
         */
        template <typename T>
        static std::string to_string(const T& n)
        {
                std::ostringstream stm;
                stm << n;
                return stm.str();
        }

        /**
         * $brief Convert argument from std::string
         *
         * @param s - source string
         *
         * @return value of T type converted from std::string
         */
        template <typename T>
        static T from_string(const std::string& s)
        {
                std::istringstream stm(s);
                T n;
                stm >> n;
                return n;
        }

        /**
         * $brief Parse json from string
         *
         * @param j - stringified json
         *
         * @return Parsed JSON element, e.g. Object, Array, etc.
         */
	template<typename T>
	static T parse_json(const std::string& j)
	{
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var v = parser.parse(j);
		T object = v.extract<T>();
		return object;
	}

        /**
         * $brief Parse json from stream
         *
         * @param i - input stream
         *
         * @return Parsed JSON element, e.g. Object, Array, etc.
         */
	template<typename T>
	static T parse_json(std::istream& i)
	{
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var v = parser.parse(i);
		T object = v.extract<T>();
		return object;
	}

        /**
         * @brief trim from start (in place)
         *
         * @param s - string to be trimmed
         */
        static void ltrim(std::string& s);

        /**
         * @brief trim from end (in place)
         *
         * @param s - string to be trimmed
         */
        static void rtrim(std::string& s);

        /**
         * @brief trim from both ends (in place)
         *
         * @param s - string to be trimmed
         */
        static void trim(std::string& s);

        /**
         * @brief Performs base64 encoding
         *
         * @param str - source string
         *
         * @return - base64 encoded string
         */
        static std::string base64_encode(const std::string& str);

        /**
         * @brief Performs base64 dencoding
         *
         * @param str - source string
         *
         * @return - decoded string
         */
        static std::string base64_decode(const std::string& str);

        /**
         * @brief Performs base64 encoding from byte vector
         *
         * @param vec - source byte vector
         *
         * @return - base64 encoded string
         */
        static std::string base64_encode(
                        const std::vector<unsigned char>& vec);

        /**
         * @brief Performs base64 encoding
         *
         * @param pkg - source package
         *
         * @return - base64 encoded string
         */
        static std::string base64_encode(default_package pkg);

        /**
         * @brief read file content
         *
         * @param p - is the path of the file
         * @param trim - if true, removes beginning and trailing whitespaces
         *
         * @return the content of the file in std::string
         */
        static std::string read_file_content(const std::string& p,
                                                        bool trim = true);

        /// @name Special member functions
private:
        utils();
        virtual ~utils();
};

namespace leviathan {
        namespace platform {

                /**
                 * @brief Specialization of to_string for JSON object. Just
                 * stringifies the provided JSON object
                 *
                 * @param n - source JSON object
                 *
                 * @return - strinfified JSON object
                 */
                template<>
                inline std::string utils::
                to_string<Poco::JSON::Object::Ptr>(
                               const Poco::JSON::Object::Ptr& n)
                {
                        poco_assert(nullptr != n);
                        std::stringstream stm;
                        n->stringify(stm);
                        return stm.str();
                }

        }
}

#endif // LEVIATHAN_PLATFORM_UTILS_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

