/**
 * @file src/platform/task.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::platform::csp::task
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_PLATFORM_CSP_TASK_HPP
#define LEVIATHAN_PLATFORM_CSP_TASK_HPP

// Headers from this project

// Headers from other projects

// Headers from third party libraries
#include <Poco/Task.h>
#include <Poco/TaskManager.h>
#include <Poco/TaskNotification.h>
#include <Poco/Logger.h>
#include <Poco/ThreadPool.h>

// Headers from standard libraries
#include <list>
#include <set>

// Forward declarations
namespace leviathan {
        namespace platform {
                namespace csp { // GARIK : delete csp namespace

                        typedef Poco::Event Control;

                        class base_input;

                        class base_output;

                        template <typename T>
                        class input;

                        template <typename T>
                        class output;

                        class task;

                        class manager;

                        template <typename T>
                        void connect(input<T>&, output<T>&);

                        template <typename T>
                        void connect(output<T>&, input<T>&);
                }
        }
}

/**
 * @brief Task class provides means to implement multithreading within the
 * application.
 *
 * To create a task user should derive a class from 
 * @ref csp::task and redefine pure virtual function @b runTask.
 * @b runTask function defined in user's class represents a segment of code
 * which performs typical job. This function will be invoked later by calling
 * function @b start of @ref csp::manager. 
 *
 * Each task is registered in csp::manager upon creation.
 *
 * csp::task provides the following interface to implement effective
 * multithreading application and synchronization between tasks.
 *
 * - Several versions of the waiting/sleeping interface are provided which allow
 * to make a task sleep when no input data is available to be processed.
 * 2 different wait interface is available.
 *      - wait() - which makes the task to sleep and wake up data is available
 *      in any input of its inputs.
 *      This function can be used to organize OR wait logic on all inputs i.e.
 *      wait (i1 || i2 || i2 || ... || iN)
 *      - wait(base_input&, ...) - makes the task to sleep on all inputs
 *      provided as argument and wake up only if all inputs have data to be
 *      processed.
 *      This function can be used to organize and wait logic on all inputs i.e.
 *      wait (i1 && i2 && i2 && ... && iN)
 *
 * - @b finish() interface is provided which deactivates all outputs of the
 * task. This means that no data will be provided by this task anymore. This allows
 * to organize propagation of finish through the application.
 *
 * Below are use case for data transmition loop between two tasks:
 * @code
 * class loop : public task
 * {
 *      bool m_initiator;
 * public:
 *      input<data> rx;
 *      output<data> tx;
 *
 *      void runTask()
 *      {
 *              data d;
 *              if (m_initiator) {
 *                      *tx = data();
 *                      tx.commit();
 *              }
 *              while (1) {
 *                     wait(rx);
 *                     poco_assert(rx.has_data());
 *                     *tx = *rx;
 *                     tx.commit();
 *                     rx.consume();
 *             }
 *             finish()
 *      }
 *      loop(bool initiator) 
 *              : task("loop")
 *              , m_initiator(initiator)
 *              , rx(this)
 *              , tx(this)
 *     {
 *     }
 * };
 *
 * // somewhere in the main()
 * loop* master = new loop(true);
 * loop* slave = new loop(false);
 * connect(masert->tx, slave->rx);
 * connect(masert->rx, slave->tx);
 * manager::start();
 * manager::join();
 *
 * @endcode
 * 
 */
class leviathan::platform::csp::task 
        : public Poco::Task
{
        friend class base_input;
        friend class base_output;
        /// @name Public type definitions
public:
        typedef std::set<base_input*> inputs;
        typedef std::set<base_output*> outputs;

        /// @name Public interface
public:
        /**
         * @brief Pure virtual function of the task.
         *
         * Each derived class should override the function which will be main
         * logic of the task.
         *
         */
        virtual void runTask() = 0;

        /// @name Protected member functions
protected:
        /**
         * @brief Wakes up the task by setting its control signal.
         */
        void wake_up();

        /**
         * @brief Resets task signal.
         */
        void reset();

        /**
         * @brief Sleeps on provided inputs.
         *
         * Makes thread to sleep on the inputs and wakes up the thread when
         * all inputs have data.
         *
         * Returns true if wait was successful and data is available to be
         * processed, otherwise false is returned which indicates that no data
         * will be available for processing, i.e. task should be finished.
         *
         * This function can be used in case if number of inputs to be wait is
         * greater than 7. Otherwise the overloaded version below can be used.
         *
         * @param fifos - list of fifos on which wait operation is performed
         */
        bool wait(const inputs& fifos);

        /**
         * @brief Sleeps on provided input.
         *
         * Returns true if wait was successful and data is available to be
         * processed, otherwise false is returned which indicates that no data
         * will be available for processing anymore for this input.
         *
         * Makes thread to sleep on the input and wakes up the task when
         * data is available in the provided input.
         *
         */
        bool wait(base_input&);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         */
        bool wait(base_input& i1, base_input& i2);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *              poco_assert(i3.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         * @param i3 - input of the task
         */
        bool wait(base_input& i1, base_input& i2, base_input&);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *              poco_assert(i3.has_data());
         *              poco_assert(i4.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         * @param i3 - input of the task
         * @param i4 - input of the task
         */
        bool wait(base_input& i1, base_input& i2, base_input& i3,
                  base_input& i4);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *              poco_assert(i3.has_data());
         *              poco_assert(i4.has_data());
         *              poco_assert(i5.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         * @param i3 - input of the task
         * @param i4 - input of the task
         * @param i5 - input of the task
         */
        bool wait(base_input& i1, base_input& i2, base_input& i3,
                  base_input& i4, base_input& i5);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *              poco_assert(i3.has_data());
         *              poco_assert(i4.has_data());
         *              poco_assert(i5.has_data());
         *              poco_assert(i6.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         * @param i3 - input of the task
         * @param i4 - input of the task
         * @param i5 - input of the task
         * @param i6 - input of the task
         */
        bool wait(base_input& i1, base_input& i2, base_input& i3,
                  base_input& i4, base_input& i5, base_input& i6);

        /**
         * @brief Sleeps on provided inputs.
         *
         * Returns true if wait was successful and data is available to be
         * processed in all inputs provided as argument. Otherwise false is
         * returned which indicates that no data will be available for
         * processing for the mentioned input(s).
         *
         * Should be used if task should be waken up when all inputs have data to
         * be processed.
         *
         * @post  if (true == wait_status) {
         *              poco_assert(i1.has_data());
         *              poco_assert(i2.has_data());
         *              poco_assert(i3.has_data());
         *              poco_assert(i4.has_data());
         *              poco_assert(i5.has_data());
         *              poco_assert(i6.has_data());
         *              poco_assert(i7.has_data());
         *        }
         *
         * @param i1 - input of the task
         * @param i2 - input of the task
         * @param i3 - input of the task
         * @param i4 - input of the task
         * @param i5 - input of the task
         * @param i6 - input of the task
         * @param i7 - input of the task
         */
        bool wait(base_input& i1, base_input& i2, base_input& i3,
                  base_input& i4, base_input& i5, base_input& i6,
                  base_input& i7);

        /**
         * @brief Makes a task to sleep on internal control signal.
         *
         * By default task's internal control signal is shared with all its
         * inputs.
         * 
         * The task will be waken up if any of the inputs will be pushed.
         *
         * Returns true if wait was successful and data is available to be
         * processed, otherwise false is returned which indicates that no data
         * will be available from any input of the task for processing.
         * This fact can be considered that the task should be finished.
         */
        bool wait();

        /**
         * @brief Adds provided input to the input list of the task.
         *
         * All inputs of the task automatically is added to the inputs list of
         * the task.
         */
        void add_to_input_list(base_input* );

        /**
         * @brief Adds provided output to the output list of the task.
         *
         * All outputs of the task automatically is added to the outputs list of
         * the task.
         */
        void add_to_output_list(base_output* );

        /**
         * @brief Adds provided input to the input list of the task.
         *
         * All inputs of the task automatically is added to the inputs list of
         * the task.
         */
        void remove_from_input_list(base_input* );

        /**
         * @brief Adds provided output to the output list of the task.
         *
         * All outputs of the task automatically is added to the outputs list of
         * the task.
         */
        void remove_from_output_list(base_output* );

        /**
         * @brief Interface to finish task gracefully and notify all dependent
         * tasks that there will not be any input from this task anymore.
         *
         * It is recommended to call this function as the last statement of
         * runTask() function in the inherited task.
         */
        void finish();

public:
        /**
         * @brief Returns control signal of the task.
         */
        Poco::SharedPtr<csp::Control>& get_control();

        /**
         * @brief Stops the task execution
         */
        void stop();

        /// @name Special member functions
public:
        /**
         * Default constructor
         *
         * Each task is added to csp::manager tasks list by default.
         * If @a managed is specified false task will not be added to the task
         * manager.
         *
         * @param name - task name
         * @param logger - task logger
         * @param managed - indicates whether the task is managed by task
         * manager or not
         */
        task(const std::string& name,
                        Poco::Logger& logger = Poco::Logger::root(),
                        bool managed = true);

        /// Destructor
        virtual ~task();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        task(const task&) = delete;

        /// This class is not assignable
        task& operator=(const task&) = delete;

        /// @name Protected member variables
protected:
        /// Internal signal of the task which can be used to sleep/wake up the
        /// task upon some event. When wait() function is called task waits for
        //  m_signal to be set/notified.
        Poco::SharedPtr<csp::Control> m_signal;

        inputs m_inputs;
        Poco::Mutex m_input_mutex;

        outputs m_outputs;
        Poco::Mutex m_output_mutex;
        Poco::Logger& m_logger;

}; // class leviathan::platform::task

/**
 * @brief Implements task management logic with own thread pool.
 *
 * It is singleton class.
 * All tasks by default are added to manager.
 * Upon manager start all registered tasks are started.
 *
 * Provides API to start, add/remove tasks.
 * NOTE 1: API is not thread safe.
 *
 * NOTE 2: Start of any task after mamager::join function call fails as
 * Poco::ThreadPool is blocked by mutex which is locked by manager::join.
 * To start task dynamically at any point/moment/time use another
 * Poco::ThreadPool.
 *
 * NOTE 3: csp::manager is implemented based on Poco::TaskManager
 * Poco::TaskManager takes ownership of each task added to its task list and
 * deletes it when task(thread) is finished.
 *
 * So it is recommended to instantiate each task as pointer and not to delete it
 * manually as Poco::TaskManager will take care about its deletion.
 * 
 */
class leviathan::platform::csp::manager
        : public Poco::TaskManager
{

        /// @name Public static interface
public:
        /**
         * @brief Starts all registered tasks.
         */
        static void start();

        /**
         * @brief Waits for finish of started tasks.
         */
        static void join();

        /**
         * @brief Stops all the tasks in task manager
         */
        static void cancel_all();

        /**
         * @brief Returns one and only instance of this class
         *
         * Creates instance If no object is created yet.
         */
        static manager* get_instance();

private:
        friend class task;

        /**
         * @brief Adds the task to manager's tasks list
         *
         * @param t - task to be added to the list
         */
        static void add_task(task* t);

        /**
         * @brief Stops and removes task from task list.
         *
         * @param t - task to be removed from the list
         */
        static void remove_task(task* t);

        /// @name Private special member functions
private:
        /**
         * @brief Constructor
         */
        manager();

        /// Destructor
         ~manager();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        manager(const manager&);

        /// This class is not assignable
        manager& operator=(const manager&);

private:
        Poco::ThreadPool m_pool;
        static manager* s_instance;

        typedef std::set<task*> tasks;
        tasks m_tasks;

        Poco::Mutex m_mutex;

}; // class leviathan::platform::manager

template <typename T>
void leviathan::platform::csp::
connect(leviathan::platform::csp::input<T>& in,
        leviathan::platform::csp::output<T>& out)
{
        in.connect(out);
}

template <typename T>
void leviathan::platform::csp::
connect(leviathan::platform::csp::output<T>& in,
        leviathan::platform::csp::input<T>& out)
{
        in.connect(out);
}

#endif // LEVIATHAN_PLATFORM_CSP_TASK_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

