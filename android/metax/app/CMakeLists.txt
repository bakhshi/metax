# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(POCO_PATH
        /opt/poco/poco1.8.1_android15
)

set(OPENSSL_PATH
        /opt/openssl/openssl1.1.0g_android15
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             metax_web_api

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/metax_web_api.cpp
             src/main/cpp/src/metax_web_api/http_server.cpp
             src/main/cpp/src/metax_web_api/db_request_handler.cpp
             src/main/cpp/src/metax_web_api/default_request_handler.cpp
             src/main/cpp/src/metax_web_api/refuse_handler.cpp
             src/main/cpp/src/metax_web_api/email_sender.cpp
             src/main/cpp/src/metax_web_api/web_api_adapter.cpp
             src/main/cpp/src/metax_web_api/websocket_request_handler.cpp
             src/main/cpp/src/metax_web_api/web_request_handler_factory.cpp
             src/main/cpp/src/metax_web_api/checked_web_request_handler_factory.cpp
             src/main/cpp/src/metax_web_api/configuration_handler.cpp
             src/main/cpp/src/platform/db_json.cpp
             src/main/cpp/src/platform/db_manager.cpp
             src/main/cpp/src/platform/default_package.cpp
             src/main/cpp/src/platform/task.cpp
             src/main/cpp/src/platform/utils.cpp
             src/main/cpp/src/metax/configuration_manager.cpp
             src/main/cpp/src/metax/backup.cpp
             src/main/cpp/src/metax/hop.cpp
             src/main/cpp/src/metax/kernel.cpp
             src/main/cpp/src/metax/key_manager.cpp
             src/main/cpp/src/metax/link.cpp
             src/main/cpp/src/metax/link_peer.cpp
             src/main/cpp/src/metax/ssl_password_handler.cpp
             src/main/cpp/src/metax/router.cpp
             src/main/cpp/src/metax/socket_reactor.cpp
             src/main/cpp/src/metax/storage.cpp
             src/main/cpp/src/metax/wallet.cpp
             src/main/cpp/src/metax/user_manager.cpp
)

add_library( poco-net-ssl
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-net-ssl

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoNetSSL.a )

add_library( poco-net
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-net

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoNet.a )


add_library( poco-foundation
             STATIC
             IMPORTED )


set_target_properties( # Specifies the target library.
             poco-foundation

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoFoundation.a)

add_library( poco-JSON
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-JSON

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoJSON.a)

add_library( poco-util
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-util

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoUtil.a)

add_library( poco-xml
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-xml

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoXML.a)

add_library( poco-crypto
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             poco-crypto

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${POCO_PATH}/lib/libPocoCrypto.a)

add_library( crypto-lib
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
             crypto-lib

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${OPENSSL_PATH}/lib/libcrypto.a)

add_library( ssl-lib
             STATIC
             IMPORTED )

set_target_properties( # Specifies the target library.
        ssl-lib

        # Specifies the parameter you want to define.
        PROPERTIES IMPORTED_LOCATION

        # Provides the path to the library you want to import.
        ${OPENSSL_PATH}/lib/libssl.a)

include_directories(
        ${POCO_PATH}/include/
        ${OPENSSL_PATH}/include/
        src/main/cpp/src/
)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                        metax_web_api
                        poco-net-ssl
                        poco-net
                        poco-crypto
                        poco-util
                        poco-xml
                        poco-JSON
                        poco-foundation
                        ssl-lib
                        crypto-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
