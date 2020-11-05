/**
 * @file src/platform/fifo.cpp
 *
 * @brief Definition of the class @ref
 * leviathan::platform::csp::fifo
 *
 * @COPYWRITE_TODO
 *
 */

// Headers from this project
#include "task.hpp"
#include "utils.hpp"

// Headers from other projects

// Headers from third party libraries
#include <Poco/Notification.h>

// Headers from standard libraries
#include <cstdio>

template <typename T>
void leviathan::platform::csp::fifo<T>::
pop()
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        poco_assert(has_data());
        m_internal.pop_front();
}

template <typename T>
const T* leviathan::platform::csp::fifo<T>::
front() const
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        poco_assert(has_data());
        return m_internal.front();
}

template <typename T>
T* leviathan::platform::csp::fifo<T>::
back()
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        return m_internal.back();
}

template <typename T>
void leviathan::platform::csp::fifo<T>::
push()
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        m_internal.push_back(new T());
        poco_assert(has_data());
        m_available.set();
        if (nullptr != m_notify) {
                m_notify->set();
        }
}

template <typename T>
bool leviathan::platform::csp::fifo<T>::
has_data() const
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        return m_internal.size() > 1;
}

template <typename T>
unsigned long  leviathan::platform::csp::fifo<T>::
size() const
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        return m_internal.size() - 1;
}

template <typename T>
bool leviathan::platform::csp::fifo<T>::
is_active() const
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        return m_active;
}

template <typename T>
void leviathan::platform::csp::fifo<T>::
deactivate()
{
        Poco::Mutex::ScopedLock lock(m_mutex);
        m_active = false;
        m_available.set();
        if (nullptr != m_notify) {
                m_notify->set();
        }
}

template <typename T>
bool leviathan::platform::csp::fifo<T>::
wait_for_data()
{
        /// If there is data there is no need to fall in the wait state.
        if (has_data()) { 
                // Poco::Event is implemented so that if set() is called before wait()
                // then upon wait function call thread will wake up immediately  without
                // sleeping
                // Resets signaled state of the object
                m_available.reset();
                return true;
        }
        /// If there is no data we need to be sure that fifo is still active
        if (is_active()) {
                m_available.wait();
                /// When fifo is triggered, its active state is returned as proof of
                /// valid data availability for wake up signal. 
                /// Fifo is triggered also when it has been deactivated, in
                /// order to waken up threads which were waiting for data.
                return is_active();
        }
        return false;
}

template <typename T>
void leviathan::platform::csp::fifo<T>::
set_external_control(Poco::SharedPtr<leviathan::platform::csp::Control>& c)
{
        m_notify = c;
}

template <typename T>
leviathan::platform::csp::fifo<T>::
fifo()
        : m_internal()
        , m_available()
        , m_active(true)
        , m_notify()
        , m_mutex()
{
        m_internal.push_back(new T());
}

template <typename T>
leviathan::platform::csp::fifo<T>::
~fifo()
{
        m_internal.clear();
}


/// csp::input class member function definition
template <typename T>
bool leviathan::platform::csp::input<T>:: 
has_data() const
{
        poco_assert(nullptr != m_src);
        return m_src->has_data();
}

template <typename T>
unsigned long leviathan::platform::csp::input<T>::
data_count() const
{
        poco_assert(nullptr != m_src);
        return m_src->size();
}

template <typename T>
bool leviathan::platform::csp::input<T>:: 
is_active() const
{
        poco_assert(nullptr != m_src);
        return m_src->is_active();
}

template <typename T>
bool leviathan::platform::csp::input<T>:: 
wait()
{
        poco_assert(nullptr != m_src);
        return m_src->wait_for_data();
}

template <typename T>
void leviathan::platform::csp::input<T>:: 
consume() 
{
        poco_assert(nullptr != m_src);
        m_src->pop();
}

template <typename T>
const T& leviathan::platform::csp::input<T>:: 
operator*() const
{
        poco_assert(nullptr != m_src);
        return *(m_src->front());
}

template <typename T>
const T* leviathan::platform::csp::input<T>:: 
operator->() const
{
        poco_assert(nullptr != m_src);
        return m_src->front();
}

template <typename T>
void leviathan::platform::csp::input<T>:: 
connect(csp::output<T>& out)
{
        poco_assert(nullptr != m_src);
        out.m_dest = m_src;
}

template <typename T>
leviathan::platform::csp::input<T>::
input(csp::task* o)
        : leviathan::platform::csp::base_input(o)
{
        poco_assert(nullptr != o);
        m_src = new csp::fifo<T>();
        m_src->set_external_control(o->get_control());
}

template <typename T>
leviathan::platform::csp::input<T>:: 
~input()
{
}

//// csp::output member functions definitions
template <typename T>
void leviathan::platform::csp::output<T>::
commit() 
{
        poco_assert(nullptr != m_dest);
        m_dest->push();
}

template <typename T>
T& leviathan::platform::csp::output<T>::
operator*()
{
        poco_assert(nullptr != m_dest);
        return *(m_dest->back());
}

template <typename T>
T* leviathan::platform::csp::output<T>::
operator->()
{
        poco_assert(nullptr != m_dest);
        return m_dest->back();
}

template <typename T>
void leviathan::platform::csp::output<T>::
connect(csp::input<T>& in)
{
        poco_assert(nullptr != in.m_src);
        m_dest = in.m_src;
}

template <typename T>
void leviathan::platform::csp::output<T>::
deactivate()
{
        poco_assert(nullptr != m_dest);
        m_dest->deactivate();
}

template <typename T>
leviathan::platform::csp::output<T>::
output(csp::task* o)
    : leviathan::platform::csp::base_output(o)
    , m_dest()
{
}

template <typename T>
leviathan::platform::csp::output<T>::
~output()
{
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:

