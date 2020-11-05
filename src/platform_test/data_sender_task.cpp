
/**
 * @file src/platform_test/data_sender_task.cpp
 *
 * @brief Implements data sender task for testing platform library
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
        namespace platform_test{
                class data_sender;
                int data_sender_task();
        }
}

/**
 * @brief Receives data from many input ports and sends through one output port
 * sequentially
 */
class leviathan::platform_test::data_sender:
		public leviathan::platform::csp::task
{
public:
        leviathan::platform::csp::input<int> rx1;
        leviathan::platform::csp::input<int> rx2;
        leviathan::platform::csp::input<int> rx3;
        leviathan::platform::csp::input<int> rx4;
        leviathan::platform::csp::input<int> rx5;
        leviathan::platform::csp::output<int> tx;

public:
        void runTask()
        {
                while (true) {
                        std::cout << this->name() << " " << std::endl;
                        if (! wait()) {
                                break;
                        }
                        if (rx1.has_data()) {
                                poco_assert(rx1.has_data());
                                std::cout << "rx1 - " << *rx1 << std::endl << std::flush;
                                *tx = *rx1;
                                rx1.consume();
                                tx.commit();
                        }
                        if (rx2.has_data()) {
                                poco_assert(rx2.has_data());
                                std::cout << "rx2 - " << *rx2 << std::endl << std::flush;
                                *tx = *rx2;
                                rx2.consume();
                                tx.commit();
                        }
                        if (rx3.has_data()) {
                                poco_assert(rx3.has_data());
                                std::cout << "rx3 - " << *rx3 << std::endl << std::flush;
                                *tx = *rx3;
                                rx3.consume();
                                tx.commit();
                        }
                        if (rx4.has_data()) {
                                poco_assert(rx4.has_data());
                                std::cout << "rx4 - " << *rx4 << std::endl << std::flush;
                                *tx = *rx4;
                                rx4.consume();
                                tx.commit();
                        } 
                        if (rx5.has_data()) {
                                poco_assert(rx5.has_data());
                                std::cout << "rx5 - " << *rx5 << std::endl << std::flush;
                                *tx = *rx5;
                                rx5.consume();
                                tx.commit();
                        }
                }
                std::cout << name() << " FINISH " << std::endl;
                finish();
        }

        data_sender(const std::string& cname) 
                : leviathan::platform::csp::task(cname.c_str())
                  , rx1(this)
                  , rx2(this)
                  , rx3(this)
                  , rx4(this)
                  , rx5(this)
                  , tx(this)
        {
        }
};

int leviathan::platform_test::data_sender_task()
{
        std::cout << "------------------------" << std::endl;
        std::cout << "Data sender task started" << std::endl;
        std::cout << "------------------------" << std::endl;

        int runtime = 2;
        namespace N = leviathan::platform_test;
        N::reader* input1 = new N::reader("Data1", runtime);
        N::reader* input2 = new N::reader("Data2", runtime);
        N::reader* input3 = new N::reader("Data3", runtime);
        N::reader* input4 = new N::reader("Data4", runtime);
        N::reader* input5 = new N::reader("Data5", runtime);
        N::writer* out = new N::writer("Output");
        N::data_sender* result = new N::data_sender("Data Sender");
        input1->tx.connect(result->rx1);
        input2->tx.connect(result->rx2);
        input3->tx.connect(result->rx3);
        input4->tx.connect(result->rx4);
        input5->tx.connect(result->rx5);
        result->tx.connect(out->rx);
        leviathan::platform::csp::manager::start();
        leviathan::platform::csp::manager::join();

        std::cout << "------------------------" << std::endl;
        std::cout << "Data sender task finished" << std::endl;
        std::cout << "------------------------\n" << std::endl;
        return 0;
}
