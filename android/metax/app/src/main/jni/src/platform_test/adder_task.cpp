
/**
 * @file src/platform_test/adder_task.cpp
 *
 * @brief Implements adder task for testing platform library
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "reader_writer.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries

// Headers from standard libraries
#include <iostream>
#include <string>
#include <cstdlib>


// Forward declarations
namespace leviathan {
        namespace platform_test {
                class adder;
                int adder_task();
        }
}

/**
 * @brief Computes some from two inputs and sends the result through ouput port
 */
class leviathan::platform_test::adder: public leviathan::platform::csp::task
{
public:
        leviathan::platform::csp::input<int> rx1;
        leviathan::platform::csp::input<int> rx2;
        leviathan::platform::csp::output<int> tx;

public:
        void runTask()
        {
                std::cout << this->name() << " STARTED " << std::endl;
                while (true) {
                        std::cout << this->name() << " " << std::endl;
                        if (!wait(rx1, rx2)) {
                                break;
                        }
                        std::cout << "rx1 and rx2 are available" << std::endl;
                        poco_assert(rx1.has_data());
                        poco_assert(rx2.has_data());
                        *tx = *rx1 + *rx2;
                        rx1.consume();
                        rx2.consume();
                        tx.commit();
                }
                finish();
        }

        adder(const std::string& cname) 
                : leviathan::platform::csp::task(cname.c_str())
                  , rx1(this)
                  , rx2(this)
                  , tx(this)
        {
        }
};

int leviathan::platform_test::adder_task()
{
        // test for a + b
        std::cout << "------------------------" << std::endl;
        std::cout << "Adder Test Case" << std::endl;
        std::cout << "------------------------" << std::endl;

        int runtime = 1000;
        namespace N = leviathan::platform_test;
        N::reader* input1 = new N::reader("Data1", runtime);
        N::reader* input2 = new N::reader("Data2", runtime);
        N::writer* out = new N::writer("Output");
        N::adder* add = new N::adder("Adder");
        input1->tx.connect(add->rx1);
        input2->tx.connect(add->rx2);
        add->tx.connect(out->rx);
        leviathan::platform::csp::manager::start();
        leviathan::platform::csp::manager::join();

        std::cout << "------------------------" << std::endl;
        std::cout << "Adder Test Case End" << std::endl;
        std::cout << "------------------------\n" << std::endl;
        return 0;
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
