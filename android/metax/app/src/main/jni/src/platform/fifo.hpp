/**
 * @file src/platform/fifo.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::platform::csp::fifo
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_CSP_FIFO_HPP
#define LEVIATHAN_PLATFORM_CSP_FIFO_HPP

// Headers from this project
#include "task.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

// Headers from standard libraries
#include <deque>


// Forward declarations
namespace leviathan {
        namespace platform {
                namespace csp {

                        class base_input;

                        class base_output;

                        template<typename T>
                        class fifo;

                        template <typename T>
                        class input;

                        template <typename T>
                        class output;

                        class task;
                }
        }
}

/**
 * @brief Provides means to communicate/transfer data between 2+ csp::tasks
 * It is half-duplex fifo, i.e. data flows in one direction.
 *
 * Fifo has internal 2 states: active and inactive.
 * If fifo is active it means data will flow through fifo, otherwise it means
 * that no data will be committed into fifo anymore.
 * By default fifo is in active state unless otherwise is specified.
 *
 * Read interface of the csp::fifo is wrapped in csp::input class and Write
 * interface is wrapped in csp::output class.
 *
 * By design csp::fifo can have multiple sources(writers) but only one
 * destination(reader). It is recommended to have one source and one destination
 * connected to fifo.
 *
 * Besides ordinary fifo interface related to data (push/pop/has_data/front)
 * also notification interface for data availability is provided.
 * Wait interface is implemented which allows to sleep on fifo until data is
 * pushed.
 *
 * Each fifo has internal control signal (csp::Control) which is used to notify
 * about data availability in it.
 * This interface can be used to wait for particular fifo for data availability.
 * csp::fifo also provides interface to register external control signal which
 * will be also triggered upon push event on fifo.
 *
 * IMPLEMENTATION DETAILS:
 * Data of the fifo is stored in deque member variable.
 * Control member variable is of Poco::Event type.
 *
 */
template <typename T>
class leviathan::platform::csp::fifo
{
        /// @name Public Type definition
public:
        typedef T value_type;

        /// @name Public interface
public:

        /// @brief Pops front data of the fifo.
        void pop();

        /// @brief Appends data to the fifo.
        void push();

        /// @brief Returns front data of the fifo.
        const T* front() const;

        /// @brief Returns front data of the fifo.
        T* back();

        /// @brief Returns true if fifo has data, otherwise false.
        bool has_data() const;

        /// @brief Returns size of the available data in fifo
        unsigned long size() const;

        /**
         * @brief Returns true if fifo has valid data, otherwise false.
         *
         * False indicates that no valid data is available in fifo and it is not
         * active anymore, i.e. no data will flow through it.
         */
        bool wait_for_data();

        /// @brief Sets external control signal.
        void set_external_control(Poco::SharedPtr<Control> &);

        /// @brief Sets active flag to false.
        void deactivate();

        /// @brief Returns active/inactive state of the fifo.
        bool is_active() const;

        /// @name Special member functions
public:
        /// Default constructor
        fifo();

        /// Destructor
        virtual ~fifo();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        fifo(const fifo&);

        /// This class is not assignable
        fifo& operator= (const fifo&);

private:
        // Buffer which handles actual data
        std::deque<Poco::SharedPtr<T> > m_internal;

        // Control to indicate that data has been pushed to fifo
        Control m_available;

        /**
         * Describes state of the fifo: active/inactive.
         * If fifo is in inactive state that means no data will be pushed
         * anymore.
         */
        bool m_active;

        // Control to notify external registered object regarding data
        // availability
        Poco::SharedPtr<Control> m_notify;

        // Mutex to make pop/push/etc function calls thread-safe
        mutable Poco::Mutex m_mutex;

}; // class leviathan::platform::csp::fifo


/// Base class for task inputs.
class leviathan::platform::csp::base_input
{
        /// @name Special member functions
public:
        /**
         * Default constructor
         */
        base_input(csp::task* o)
                : m_owner(o)
        {
                poco_assert(nullptr != m_owner);
                m_owner->add_to_input_list(this);
        }

        /// Destructor
        virtual ~base_input()
        {
                if (nullptr != m_owner) {
                        m_owner->remove_from_input_list(this);
                }
                m_owner = nullptr;
        }

        /**
         * @brief Interface to wait for input data be available.
         *
         * Return value indicates whether the wait was successful or not.
         * True indicates that valid data is available in input.
         * False indicates that valid data is not available and input is not
         * active anymore.
         */
        virtual bool wait() = 0;

        /// @brief Interface to check whether data available in input
        virtual bool has_data() const = 0;

        /// @brief Interface to get available data count
        virtual unsigned long data_count() const = 0;

        /// @brief Interface to check whether input is still active or not.
        virtual bool is_active() const = 0;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        base_input(const base_input&) = delete;

        /// This class is not assignable
        base_input& operator=(const base_input&) = delete;

        /// @name Private member variables
protected:
        csp::task* m_owner;
};

/// Base class for task outputs.
class leviathan::platform::csp::base_output
{
        /// @name Special member functions
public:
        /**
         * Default constructor
         */
        base_output(csp::task* o): m_owner(o)
        {
                poco_assert(nullptr != m_owner);
                m_owner->add_to_output_list(this);
        }

        /// Destructor
        virtual ~base_output()
        {
                if (nullptr != m_owner) {
                        m_owner->remove_from_output_list(this);
                }
                m_owner = nullptr;
        }

        /**
         * @brief Interface to deactivate output. 
         *
         * If the output is deactivated it means that no data will be pushed to
         * output anymore.
         */
        virtual void deactivate() = 0;

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        base_output(const base_output&) = delete;

        /// This class is not assignable
        base_output& operator=(const base_output&) = delete;

        /// @name Private member variables
protected:
        // Owner task of the output
        csp::task* m_owner;
};


/**
 * @brief Provides interface to read data from fifo
 *
 * By means of csp::input csp::task implements communication with external
 * world in terms of getting data from other tasks.
 *
 * Each input of the task is added to the task inputs list by default.
 */
template <typename T>
class leviathan::platform::csp::input: 
        public leviathan::platform::csp::base_input
{
        friend class output<T>;
public:
        typedef T value_type;
        /// @name Special member functions
public:
        /**
         * @brief Default constructor
         */
        input(csp::task* o);

        /// @brief Destructor
        virtual ~input();

        /// @brief Returns true if there is data in input, false otherwise
        bool has_data() const;

        /// @brief returns count of available data
        unsigned long data_count() const;

        /**
         * @brief Returns true if input is still active, i.e. data will flow
         * by it yet, otherwise false is returned.
         */
        bool is_active() const;

        /**
         * @brief Waits for input data to be available.
         *
         * Returns true if there is valid data in input, otherwise false is
         * returned.
         */
        bool wait();

        /// @brief Pops data from input.
        void consume() ;

        /// @brief Returns top data of input
        const T& operator*() const;

        /// @brief Returns top data of input
        const T* operator->() const;

        /// @brief Connects input to the specified output
        void connect(csp::output<T>& out);

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        input(const input&) = delete;

        /// This class is not assignable
        input& operator=(const input&) = delete;

        /// @name Private member variables
private:
        typedef csp::fifo<T> FIFO;

        // Source fifo of the input.
        Poco::SharedPtr<FIFO> m_src;
};

/**
 * @brief Provides interface to write data into fifo.
 *
 * By means of csp::output csp::task implements communication with external
 * world in terms of providing data to other tasks.
 *
 * Each output of the task is added to the task outputs list by default.
 */
template <typename T>
class leviathan::platform::csp::output: 
        public leviathan::platform::csp::base_output
{
        friend class input<T>;
public:
        typedef T value_type;

        /// @name Special member functions
public:
        /**
         * @brief Default constructor
         */
        output(csp::task* o);

        /// @brief Destructor
        virtual ~output();

        /**
         * @brief Commits the last slot as valid one to the output.
         */
        void commit() ;

        /// @brief Returns the last slot of output
        T& operator*();

        /// @brief Returns the last slot of output
        T* operator->();

        /// @brief Connects output to the specified input
        void connect(csp::input<T>&);

        /**
         * @brief Deactivates output, which indicates that no data will be
         * provided anymore to this output.
         */
        void deactivate();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        output(const output&) = delete;

        /// This class is not assignable
        output& operator=(const output&) = delete;

        /// @name Private member variables
private:
        // Destination fifo of the output.
        // output DOES NOT OWN the fifo.
        typedef csp::fifo<T> FIFO;
        Poco::SharedPtr<FIFO> m_dest;
};

#include "fifo.txx"

#endif // LEVIATHAN_PLATFORM_CSP_FIFO_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

