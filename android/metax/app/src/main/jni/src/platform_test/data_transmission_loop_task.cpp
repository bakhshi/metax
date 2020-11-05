
/**
 * @file src/platform_test/data_transmission_loop_task.cpp
 *
 * @brief Implements data transmission loop task for testing platform library
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
#include <iostream>
#include <string>

// Forward declarations
namespace leviathan {
        namespace platform_test{
                class loop;
                struct data;
                int data_transmission_looper_task();
        }
}

/**
 * @brief Structure for holding test data
 */
struct leviathan::platform_test::data
{
        int layer;
        char name[8];
        short port;
};

/**
 * @brief Task which works in two modes:
 * master - sends value 1 in the beginning of ruTask then sends data received
 * in input, repeats this counter times provided in constructor
 * slave - reads input and sends the data through output port
 */
class leviathan::platform_test::loop : public leviathan::platform::csp::task
{
        bool m_initiator;
        int  counter;
        public:
        leviathan::platform::csp::input<data> rx;
        leviathan::platform::csp::output<data> tx;

        void runTask()
        {
                if (m_initiator) {
                        (*tx).layer = 1;
                        tx.commit();
                }
                while (! m_initiator || 0 != counter) {
                        std::cout << this->name() << std::endl;
                        if (!wait(rx)) {
                                break;
                        }
                        poco_assert(rx.has_data());
                        *tx = *rx;
                        rx.consume();
                        tx.commit();
                        --counter;
                }
                finish();
        }
        loop(bool initiator, const std::string& cname, int c) 
                  : leviathan::platform::csp::task(cname.c_str())
                  , m_initiator(initiator)
                  , counter(c)
                  , rx(this)
                  , tx(this)
        {
        }
};

int leviathan::platform_test::data_transmission_looper_task()
{
        std::cout << "------------------------" << std::endl;
        std::cout << "Data transmission loop between two tasks started" << std::endl;
        std::cout << "------------------------\n" << std::endl;
        // test for transmitting data between two tasks 
        namespace N = leviathan::platform_test;
        N::loop* master = new N::loop(true, "master", 10);
        N::loop* slave = new N::loop(false, "slave", 10);
        master->rx.connect(slave->tx);
        master->tx.connect(slave->rx);
        leviathan::platform::csp::manager::start();
        leviathan::platform::csp::manager::join();
        std::cout << "------------------------" << std::endl;
        std::cout << "Data transmission loop between two tasks finished" << std::endl;
        std::cout << "------------------------" << std::endl;
        return 0;
}
