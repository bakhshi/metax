/**
 * @file src/metax_web_api/main.cpp
 *
 * @brief implements main function to created executable
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project

// Headers from other projects
#include "http_server_app.hpp"

// Headers from third party libraries

// Headers from standard libraries


int main(int argc, char** argv)
{
        (void)argc;
        (void)argv;
        leviathan::metax_web_api::http_server_app app;
        return app.run(argc, argv);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

