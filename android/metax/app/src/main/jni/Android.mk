LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := metax_web_api

LOCAL_SRC_FILES := \
	src/metax/backup.cpp \
	src/metax/configuration_manager.cpp \
	src/metax/hop.cpp \
	src/metax/kernel.cpp \
	src/metax/key_manager.cpp \
	src/metax/link.cpp \
	src/metax/link_peer.cpp \
	src/metax/router.cpp \
	src/metax/socket_reactor.cpp \
	src/metax/ssl_password_handler.cpp \
	src/metax/storage.cpp \
	src/metax/user_manager.cpp \
	src/metax/wallet.cpp \
	src/platform/db_json.cpp \
	src/platform/db_manager.cpp \
	src/platform/default_package.cpp \
	src/platform/task.cpp \
	src/platform/utils.cpp \
	src/metax_web_api/checked_web_request_handler_factory.cpp \
	src/metax_web_api/configuration_handler.cpp \
	src/metax_web_api/db_request_handler.cpp \
	src/metax_web_api/default_request_handler.cpp \
	src/metax_web_api/email_sender.cpp \
	src/metax_web_api/http_server.cpp \
	src/metax_web_api/http_server_app.cpp \
	src/metax_web_api/main.cpp \
	src/metax_web_api/refuse_handler.cpp \
	src/metax_web_api/web_api_adapter.cpp \
	src/metax_web_api/web_request_handler_factory.cpp \
	src/metax_web_api/websocket_request_handler.cpp


LOCAL_CPP_FEATURES := rtti exceptions

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/

LOCAL_CPPFLAGS := -std=c++11 -Wall -fPIE \
	-I /opt/poco-1.9.4/android16-arm/include \
	-I /opt/openssl_1.1.1d/android16-arm/include


#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like
LOCAL_LDLIBS := -fPIE -pie \
	-L /opt/poco-1.9.4/android16-arm/lib \
	-L /opt/openssl_1.1.1d/android16-arm/lib \
	-lPocoNetSSL \
	-lPocoNet \
	-lPocoCrypto \
	-lPocoUtil \
	-lPocoJSON \
	-lPocoXML \
	-lPocoFoundation \
	-lssl \
	-lcrypto \
	-L$(SYSROOT)/usr/lib -llog

include $(BUILD_EXECUTABLE)  
