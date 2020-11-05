#include <jni.h>
#include <string>

#include <metax/hop.hpp>
#include <metax_web_api/http_server.hpp>
#include <metax_web_api/web_api_adapter.hpp>
#include <Poco/File.h>
#include <cassert>

leviathan::metax_web_api::http_server* g_server = nullptr;

extern "C"
JNIEXPORT void
JNICALL Java_am_leviathan_metax_StartMetaxService_startWebServerAndMetax(JNIEnv *env, jclass type, jstring dir, jstring file)
try {

    std::string dir_path = env->GetStringUTFChars(dir, NULL);
    std::string conf_file = env->GetStringUTFChars(file, NULL);
    std::string cfg_path = dir_path + conf_file;
    __android_log_write(ANDROID_LOG_DEBUG, "LEVIATHAN", ("Starting Metax with config file: " + cfg_path).c_str());
    namespace M = leviathan::metax;
    namespace CSP = leviathan::platform::csp;
    namespace VST = leviathan::metax_web_api;
    leviathan::platform::utils::tmp_dir = dir_path + "/tmp/";
    Poco::File td(leviathan::platform::utils::tmp_dir);
    td.createDirectories();
    M::hop node(cfg_path, dir_path + '/');
    VST::web_api_adapter* ims = VST::web_api_adapter::get_instance();
    ims->set_configuration(cfg_path);
    assert(0 != node.rx);
    assert(0 != node.tx);
    CSP::connect(ims->metax_tx, *node.rx);
    CSP::connect(ims->metax_rx, *node.tx);
    CSP::manager::start();
    g_server = new leviathan::metax_web_api::http_server(cfg_path);
    //CSP::manager::join();
} catch (const Poco::AssertionViolationException& e) {
        __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", ("Fatal error. " + e.displayText()).c_str());
} catch (const Poco::Exception& e) {
        __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", ("Fatal error. " + e.displayText()).c_str());
} catch (...) {
        __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", "Fatal error. Cannot launch Metax");
}

/*
 * Below function is for Stopping current running Metax server.
 * This functionality is currently needed for new camera/doorbell setup.
 * (Redmine ticket: https://redmine.yerevak.com/issues/9479).
 */
extern "C"
JNIEXPORT void
JNICALL Java_am_leviathan_metax_StartMetaxService_stopMetaxServer(JNIEnv *env, jclass type)
try {
    __android_log_write(ANDROID_LOG_DEBUG, "LEVIATHAN", "Stopping Metax Server");
    namespace CSP = leviathan::platform::csp;
    namespace VST = leviathan::metax_web_api;
    g_server->stop();
    delete g_server;
    g_server = nullptr;
    CSP::manager::cancel_all();
    CSP::manager::join();
    VST::web_api_adapter::delete_instance();
    __android_log_write(ANDROID_LOG_DEBUG, "LEVIATHAN", "Stopping Metax Server: DONE");
} catch (const Poco::AssertionViolationException& e) {
    __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", ("Fatal error. " + e.displayText()).c_str());
} catch (const Poco::Exception& e) {
    __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", ("Fatal error. " + e.displayText()).c_str());
} catch (...) {
    __android_log_write(ANDROID_LOG_ERROR, "LEVIATHAN", "Fatal error. Cannot Stop Metax server");
}
