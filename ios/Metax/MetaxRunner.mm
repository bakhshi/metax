//
//  MetaxRunner.m
//  Metax
//
//  Created by Raffi Knyazyan on 5/30/18.
//  Copyright © 2018 Instigate Mobile. All rights reserved.
//

#include "MetaxRunner.h"

// Headers from other projects
#include "src/metax/hop.hpp"
#include "src/metax_web_api//http_server.hpp"
#include "src/metax_web_api/web_api_adapter.hpp"

// Headers from third party libraries
#include <Poco/Util/Option.h>
#include <Poco/File.h>

// Headers from standard libraries
#include <iostream>
#include <cstring>


@implementation MetaxRunner

std::string g_cfg_path;
leviathan::metax_web_api::http_server* g_server = nullptr;

+ (void)startMetax {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *dir_base_path = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
    dir_base_path = [dir_base_path stringByDeletingLastPathComponent];
    std::string dir_path([dir_base_path cStringUsingEncoding:NSUTF8StringEncoding]);
    NSString *cfg_base_path = [[NSBundle mainBundle] pathForResource:@"config" ofType:@"xml"];
    assert(nil != cfg_base_path);
    g_cfg_path = [cfg_base_path cStringUsingEncoding:NSUTF8StringEncoding];
    namespace M = leviathan::metax;
    namespace CSP = leviathan::platform::csp;
    namespace MWA = leviathan::metax_web_api;
    leviathan::platform::utils::tmp_dir = dir_path + "/tmp/";
    M::hop node(g_cfg_path, dir_path + '/');
    MWA::web_api_adapter* ims = MWA::web_api_adapter::get_instance();
    ims->set_configuration(g_cfg_path);
    assert(0 != node.rx);
    assert(0 != node.tx);
    CSP::connect(ims->metax_tx, *node.rx);
    CSP::connect(ims->metax_rx, *node.tx);
    CSP::manager::start();
    g_server = new leviathan::metax_web_api::http_server(g_cfg_path);
}

+ (void)stopMetax {
    namespace CSP = leviathan::platform::csp;
    namespace VST = leviathan::metax_web_api;
    g_server->stop();
    delete g_server;
    g_server = nullptr;
    CSP::manager::cancel_all();
    CSP::manager::join();
    VST::web_api_adapter::delete_instance();
}


+ (void) restartServer {
    delete g_server;
    g_server = new leviathan::metax_web_api::http_server(g_cfg_path);
}

@end
