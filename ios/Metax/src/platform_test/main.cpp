/**
 * @file src/platform_test/main.cpp
 *
 * @brief Test of the class 
 * @ref leviathan::platform::csp::task
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from other projects

// Headers from third party libraries

// Headers from standard libraries

namespace leviathan {
        namespace platform_test {
                extern int adder_task();
                extern int data_sender_task();
                extern int data_transmission_looper_task();
                extern void test_db_json();
        }
}

int main()
{
        leviathan::platform_test::test_db_json();
        leviathan::platform_test::adder_task();
        leviathan::platform_test::data_sender_task();
        leviathan::platform_test::data_transmission_looper_task();

        return 0;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

