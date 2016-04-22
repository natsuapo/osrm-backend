#ifndef OSMIUM_THREAD_QUEUE_HPP
#define OSMIUM_THREAD_QUEUE_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2016 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility> // IWYU pragma: keep (for std::move)

namespace osmium {

    namespace thread {

        static const std::chrono::milliseconds full_queue_sleep_duration { 10 }; // XXX

        /**
         *  A thread-safe queue.
         */
        template <typename T>
        class Queue {

            /// Maximum size of this queue. If the queue is full pushing to
            /// the queue will block.
            const size_t m_max_size;

            /// Name of this queue (for debugging only).
            const std::string m_name;

            mutable std::mutex m_mutex;

            std::queue<T> m_queue;

            /// Used to signal readers when data is available in the queue.
            std::condition_variable m_data_available;

            std::atomic<bool> m_done;

#ifdef OSMIUM_DEBUG_QUEUE_SIZE
            /// The largest size the queue has been so far.
            size_t m_largest_size;

            /// The number of times push() was called on the queue.
            std::atomic<int> m_push_counter;

            /// The number of times the queue was full and a thread pushing
            /// to the queue was blocked.
            std::atomic<int> m_full_counter;
#endif

        public:

            /**
             * Construct a multithreaded queue.
             *
             * @param max_size Maximum number of elements in the queue. Set to
             *                 0 for an unlimited size.
             * @param name Optional name for this queue. (Used for debugging.)
             */
            explicit Queue(size_t max_size = 0, const std::string& name = "") :
                m_max_size(max_size),
                m_name(name),
                m_mutex(),
                m_queue(),
                m_data_available(),
                m_done(false)
#ifdef OSMIUM_DEBUG_QUEUE_SIZE
                ,
                m_largest_size(0),
                m_push_counter(0),
                m_full_counter(0)
#endif
            {
            }

            ~Queue() {
                shutdown();
#ifdef OSMIUM_DEBUG_QUEUE_SIZE
                std::cerr << "queue '" << m_name << "' with max_size=" << m_max_size << " had largest size " << m_largest_size << " and was full " << m_full_counter << " times in " << m_push_counter << " push() calls\n";
#endif
            }

            /**
             * Push an element onto the queue. If the queue has a max size, this
             * call will block if the queue is full.
             */
            void push(T value) {
#ifdef OSMIUM_DEBUG_QUEUE_SIZE
                ++m_push_counter;
#endif
                if (m_max_size) {
                    while (size() >= m_max_size) {
                        std::this_thread::sleep_for(full_queue_sleep_duration);
#ifdef OSMIUM_DEBUG_QUEUE_SIZE
                        ++m_full_counter;
#endif
                    }
                }
                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.push(std::move(value));
#ifdef OSMIUM_DEBUG_QUEUE_SIZE
                if (m_largest_size < m_queue.size()) {
                    m_largest_size = m_queue.size();
                }
#endif
                m_data_available.notify_one();
            }

            void shutdown() {
                m_done = true;
                m_data_available.notify_all();
            }

            void wait_and_pop(T& value) {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_data_available.wait(lock, [this] {
                    return !m_queue.empty() || m_done;
                });
                if (!m_queue.empty()) {
                    value = std::move(m_queue.front());
                    m_queue.pop();
                }
            }

            void wait_and_pop_with_timeout(T& value) {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!m_data_available.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_queue.empty() || m_done;
                })) {
                    return;
                }
                if (!m_queue.empty()) {
                    value = std::move(m_queue.front());
                    m_queue.pop();
                }
            }

            bool try_pop(T& value) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_queue.empty()) {
                    return false;
                }
                value = std::move(m_queue.front());
                m_queue.pop();
                return true;
            }

            bool empty() const {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_queue.empty();
            }

            size_t size() const {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_queue.size();
            }

        }; // class Queue

    } // namespace thread

} // namespace osmium

#endif // OSMIUM_THREAD_QUEUE_HPP
