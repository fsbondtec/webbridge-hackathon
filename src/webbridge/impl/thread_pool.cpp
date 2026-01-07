#include "thread_pool.h"

namespace webbridge {

namespace config {

// Globale Konfigurationsvariable
static size_t g_thread_pool_size = 0; // 0 = auto

void set_thread_pool_size(size_t num_threads) {
    g_thread_pool_size = num_threads;
}

size_t get_thread_pool_size() {
    return g_thread_pool_size;
}

} // namespace config

namespace impl {

thread_pool& get_thread_pool() {
    // Lazy initialization mit konfigurierter Größe
    static thread_pool pool(config::get_thread_pool_size());
    return pool;
}

} // namespace impl
} // namespace webbridge
