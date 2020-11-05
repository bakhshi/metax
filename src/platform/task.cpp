/**
 * @file src/platform/task.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::platform::csp::task
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "task.hpp"
#include "fifo.hpp"
#include "utils.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Observer.h>

// Headers from standard libraries
#include <set>

void leviathan::platform::csp::task::
wake_up()
{
        poco_assert(0 != m_signal);
        m_signal->set();
}

void leviathan::platform::csp::task::
reset()
{
        poco_assert(0 != m_signal);
        m_signal->reset();
}

bool leviathan::platform::csp::task::
wait()
{
        /// TODO: Aregnaz break down this function
        if (isCancelled()) {
                manager* mgr = manager::get_instance();
                poco_assert(0 != mgr);
                Poco::Mutex::ScopedLock lock(mgr->m_mutex);
                m_input_mutex.lock();
                m_inputs.clear();
                m_input_mutex.unlock();
                return false;
        }
        m_input_mutex.lock();
        inputs::iterator b = m_inputs.begin();
        inputs::iterator e = m_inputs.end();
        for (; b != e; ++b) {
                if ((*b)->has_data()) {
                        m_signal->reset();
                        m_input_mutex.unlock();
                        return true;
                }
        }
        m_input_mutex.unlock();
        if (m_inputs.empty()) {
                return false;
        }
        poco_assert(0 != m_signal);
        m_signal->wait();
        m_input_mutex.lock();
        b = m_inputs.begin();
        typedef std::list<inputs::value_type> CUL;
        CUL clean_up_list;
        for (; b != e; ++b) {
                if (! (*b)->is_active() && !(*b)->has_data()) {
                        clean_up_list.push_back(*b);
                }
        }
        CUL::iterator cb = clean_up_list.begin();
        CUL::iterator ce = clean_up_list.end();
        for (; cb != ce; ++cb) {
                m_inputs.erase(*cb);
        }
        m_input_mutex.unlock();
        if (! clean_up_list.empty()) {
            m_signal->reset();
            return wait();
        }
        return true;
}

bool leviathan::platform::csp::task::
wait(const inputs& in_list)
{
        inputs::const_iterator b = in_list.begin();
        inputs::const_iterator e = in_list.end();
        for (; b != e; ++b) {
                if(!(*b)->wait()) {
                        return false;
                }
        }
        return true;
}

bool leviathan::platform::csp::task::
wait(base_input& i1)
{
        return i1.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2)
{
        if (! i1.wait()) {
                return false;
        }
        return i2.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2, base_input& i3)
{
        if (!i1.wait()) {
                return false;
        }
        if (!i2.wait()) {
                return false;
        }
        return i3.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2, base_input& i3,
     base_input& i4)
{
        if (! i1.wait()) {
                return false;
        }
        if (! i2.wait()) {
                return false;
        }
        if (! i3.wait()) {
                return false;
        }
        return i4.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2, base_input& i3,
     base_input& i4, base_input& i5)
{
        if (! i1.wait()) {
                return false;
        }
        if (! i2.wait()) {
                return false;
        }
        if (! i3.wait()) {
                return false;
        }
        if (! i4.wait()) {
                return false;
        }
        return i5.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2, base_input& i3,
     base_input& i4, base_input& i5, base_input& i6)
{
        if (! i1.wait()) {
                return false;
        }
        if (! i2.wait()) {
                return false;
        }
        if (! i3.wait()) {
                return false;
        }
        if (! i4.wait()) {
                return false;
        }
        if (! i5.wait()) {
                return false;
        }
        return i6.wait();
}

bool leviathan::platform::csp::task::
wait(base_input& i1, base_input& i2, base_input& i3,
     base_input& i4, base_input& i5, base_input& i6,
     base_input& i7)
{
        if (! i1.wait()) {
                return false;
        }
        if (! i2.wait()) {
                return false;
        }
        if (! i3.wait()) {
                return false;
        }
        if (! i4.wait()) {
                return false;
        }
        if (! i5.wait()) {
                return false;
        }
        if (! i6.wait()) {
                return false;
        }
        return i7.wait();
}

void leviathan::platform::csp::task::
add_to_input_list(base_input* i)
{
        poco_assert(0 != i);
        Poco::Mutex::ScopedLock lock(m_input_mutex);
        m_inputs.insert(i);
}

void leviathan::platform::csp::task::
add_to_output_list(base_output* o)
{
        poco_assert(0 != o);
        Poco::Mutex::ScopedLock lock(m_output_mutex);
        m_outputs.insert(o);
}

void leviathan::platform::csp::task::
remove_from_input_list(base_input* i)
{
        poco_assert(0 != i);
        Poco::Mutex::ScopedLock lock(m_input_mutex);
        m_inputs.erase(i);
}

void leviathan::platform::csp::task::
remove_from_output_list(base_output* o)
{
        poco_assert(0 != o);
        Poco::Mutex::ScopedLock lock(m_output_mutex);
        m_outputs.erase(o);
}

void leviathan::platform::csp::task::
finish()
{
        METAX_INFO("FINISHING");
        wait(m_inputs);
        Poco::Mutex::ScopedLock lock(m_output_mutex);
        outputs::const_iterator b = m_outputs.begin();
        outputs::const_iterator e = m_outputs.end();
        for (; b != e; ++b) {
                (*b)->deactivate();
        }
        METAX_INFO("FINISHED");
}

Poco::SharedPtr <leviathan::platform::csp::Control> &
leviathan::platform::csp::task::
get_control()
{
        return m_signal;
}

void leviathan::platform::csp::task::
stop()
{
        METAX_TRACE(__FUNCTION__);
        cancel();
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::platform::csp::task::
task(const std::string& n, Poco::Logger& l, bool managed)
        : Poco::Task(n) 
        , m_inputs()
        , m_logger(l)
{
        m_signal = new csp::Control();
        if (managed) {
                csp::manager::add_task(this);
        } 
}

leviathan::platform::csp::task::
~task()
{
        csp::manager::remove_task(this);
}

/**
 * Default Manager definition
 */
void leviathan::platform::csp::manager::
start()
{
        manager* mgr = manager::get_instance();
        poco_assert(0 != mgr);
        Poco::Mutex::ScopedLock lock(mgr->m_mutex);
        tasks::iterator b = mgr->m_tasks.begin();
        for (; b != mgr->m_tasks.end(); ++b) {
                poco_assert(Poco::Task::TASK_IDLE == (*b)->state()); 
                try {
                        mgr->Poco::TaskManager::start(*b);
                } catch (...) { // no more threads are available in ThreadPool
                        poco_assert(false); // TODO Add appropriate handling
                        break;
                }
        }
}

void leviathan::platform::csp::manager::
join()
{
        manager* mgr = manager::get_instance();
        poco_assert(0 != mgr);
        mgr->joinAll();
}

void leviathan::platform::csp::manager::
add_task(task* t)
{
        manager* mgr = manager::get_instance();
        poco_assert(0 != mgr);
        poco_assert(0 != t);
        Poco::Mutex::ScopedLock lock(mgr->m_mutex);
        mgr->m_tasks.insert(t);
}

void leviathan::platform::csp::manager::
cancel_all()
{
        manager* mgr = manager::get_instance();
        poco_assert(0 != mgr);
        Poco::Mutex::ScopedLock lock(mgr->m_mutex);
        tasks::iterator b = mgr->m_tasks.begin();
        for (; b != mgr->m_tasks.end(); ++b) {
                (*b)->stop();
        }
}

void leviathan::platform::csp::manager::
remove_task(task* t)
{
        manager* mgr = get_instance();
        poco_assert(0 != mgr);
        poco_assert(0 != t);
        Poco::Mutex::ScopedLock lock(mgr->m_mutex);
        tasks::iterator it = mgr->m_tasks.find(t);
        if (mgr->m_tasks.end() == it) {
                return;
        }
        poco_assert(Poco::Task::TASK_FINISHED == (*it)->state()
                        || Poco::Task::TASK_CANCELLING == (*it)->state());
        // There is no necessity to erase the task form TaskManager,
        // because if the task is finished properly, 
        // TaskManager::taskFinished() is called
        mgr->m_tasks.erase(it);
}

leviathan::platform::csp::manager*
leviathan::platform::csp::manager::
get_instance()
{
        static manager instance;
        return &instance;
}

leviathan::platform::csp::manager::
manager()
        : Poco::TaskManager(m_pool)
        , m_pool("dynamic")
        , m_tasks()
{
}

leviathan::platform::csp::manager::
~manager()
{
        m_tasks.clear();
}

leviathan::platform::csp::manager*
leviathan::platform::csp::manager::s_instance = nullptr;

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

