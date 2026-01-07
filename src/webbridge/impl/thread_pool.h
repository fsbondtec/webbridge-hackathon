#pragma once

/**
 * WebBridge Thread Pool Configuration
 * 
 * Dieser Header ermöglicht die Konfiguration des Thread-Pools für Async-Calls.
 * 
 * VERWENDUNG:
 * -----------
 * Vor dem ersten WebBridge-Call (z.B. in main.cpp):
 * 
 *   #include "webbridge/impl/thread_pool.h"
 *   
 *   // Optional: Anzahl Worker-Threads setzen (Default: CPU-Kerne)
 *   webbridge::config::set_thread_pool_size(8);
 *   
 *   // Später: Thread-Pool holen
 *   auto& pool = webbridge::impl::get_thread_pool();
 * 
 * WAS PASSIERT BEI MEHR ANFRAGEN ALS THREADS?
 * -------------------------------------------
 * Der Thread-Pool verwendet eine Task-Queue (FIFO).
 * 
 * Beispiel: Pool mit 4 Threads, 10 gleichzeitige Async-Calls:
 * 
 *   1. Calls 1-4 werden sofort von den 4 Worker-Threads verarbeitet
 *   2. Calls 5-10 werden in die Queue eingereiht und warten
 *   3. Sobald Thread 1 fertig ist, nimmt er Call 5 aus der Queue
 *   4. Sobald Thread 2 fertig ist, nimmt er Call 6 aus der Queue
 *   ... und so weiter
 * 
 * VORTEILE gegenüber std::thread().detach():
 * ------------------------------------------
 * - Keine Thread-Erstellung pro Call (~50-100µs gespart pro Async-Call!)
 * - Begrenzte Thread-Anzahl verhindert Thread-Explosion
 * - Bessere CPU-Cache-Nutzung durch Thread-Wiederverwendung
 * - Kontrollierte Last auf dem System
 * 
 * NACHTEILE / TRADE-OFFS:
 * -----------------------
 * - Bei sehr langen Tasks können kurze Tasks blockiert werden
 * - Queue kann bei Überlast wachsen (Speicherverbrauch)
 * - Bei zu wenigen Threads: höhere Latenz unter Last
 * 
 * EMPFOHLENE POOL-GRÖSSEN:
 * ------------------------
 * - CPU-bound Tasks: std::thread::hardware_concurrency() (Default)
 * - I/O-bound Tasks: 2x bis 4x hardware_concurrency()
 * - Mixed Workload: hardware_concurrency() + 2
 */

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

namespace webbridge::impl {

void set_thread_pool_size(size_t num_threads);
size_t get_thread_pool_size();


// =============================================================================
// Simple Thread Pool Implementation
// =============================================================================

class thread_pool {
public:
    explicit thread_pool(size_t num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4; // Fallback
        }
        
        m_stop = false;
        m_workers.reserve(num_threads);
        
        for (size_t i = 0; i < num_threads; ++i) {
            m_workers.emplace_back([this]() {
                worker_loop();
            });
        }
    }
    
    ~thread_pool() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_condition.notify_all();
        
        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // Non-copyable, non-movable
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;
    
    /**
     * Submit a task to the pool.
     * The task will be executed by one of the worker threads.
     * If all workers are busy, the task is queued (FIFO).
     */
    void submit(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_condition.notify_one();
    }
    
    /**
     * Returns the number of worker threads.
     */
    size_t size() const {
        return m_workers.size();
    }
    
    /**
     * Returns the approximate number of pending tasks in the queue.
     */
    size_t pending() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_tasks.size();
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this]() {
                    return m_stop || !m_tasks.empty();
                });
                
                if (m_stop && m_tasks.empty()) {
                    return;
                }
                
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            
            // Execute task outside the lock
            task();
        }
    }
    
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;
};

// =============================================================================
// Global Thread Pool Access
// =============================================================================

/**
 * Returns the global thread pool instance.
 * Creates it on first call with the configured size.
 */
thread_pool& get_thread_pool();

} // namespace impl
