
/**
 * @file src/platform_test/reader_writer.hpp
 *
 * @brief Implements reader/writer tasks for testing platform library.
 *
 * COPYWRITE_TODO
 *
 */


#ifndef LEVIATHAN_PLATFORM_TEST_READER_WRITER_HPP
#define LEVIATHAN_PLATFORM_TEST_READER_WRITER_HPP

// Headers from this project
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from other projects

// Headers from third party libraries

// Headers from standard libraries
#include <iostream>
#include <string>
#include <cstdlib>

#define RANGE 1000
#define SLEEP 1000

// Forward declarations
namespace leviathan {
        namespace platform_test {
                class reader;
                class writer;
        }
}

/**
 * @brief Generates random numbers specified by constructor and sends them
 * through output port.
 */
class leviathan::platform_test::reader: public leviathan::platform::csp::task
{
        /// @name Public interface
public:
        void runTask()
        {              
                std::cout << this->name() << " STARTED " << std::endl;
                while (0 != counter) {
                        *tx = std::rand() % RANGE;
                        std::cout << this->name() << " ----- " << *tx << std::endl << std::flush;
                        tx.commit();
                        --counter;
                        //sleep(SLEEP);
                }
                std::cout << name() << " FINISH " << std::endl;
                finish();
        }

        /// @name Private data
private:
        int counter;

        /// @name Public members
public:
        leviathan::platform::csp::output<int> tx;

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        reader(const std::string& cname, int c) 
                : leviathan::platform::csp::task(cname.c_str())
                , counter(c)
                , tx(this)
        {
        }
};

/**
 * @brief Receives numbers from input port and prints in standard output.
 */
class leviathan::platform_test::writer: public leviathan::platform::csp::task
{
        /// @name Public interface
public:
        void runTask()
        {              
                std::cout << this->name() << " STARTED " << std::endl;
                while (true) {
                        if(! wait(rx)) {
                                break;
                        }
                        poco_assert(rx.has_data());
                        std::cout << this->name() << " " << *rx << std::endl;
                        rx.consume();
                        //sleep(SLEEP);
                }
                std::cout << name() << " FINISH " << std::endl;
                finish();
        }

        /// @name Public members
public:
        leviathan::platform::csp::input<int> rx;

        /// @name Special member functions
public:
        /**
         * @brief Constructor
         */
        writer(const std::string& cname) 
                : leviathan::platform::csp::task(cname.c_str())
                , rx(this)
        {
                task::add_to_input_list(&rx);
        }
};

#endif // LEVIATHAN_PLATFORM_TEST_READER_WRITER_HPP


// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
