#include <atomic>
#include <thread>

#include "context.hpp"
#include "input_handler.hpp"
#include "logger.hpp"

extern std::atomic<bool> running(true);
std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false, std::memory_order_relaxed);
}

int main() {
    signal(SIGINT, signalHandler);
    Logger::instance().enableFileOutput("log.txt");
    std::thread monitor_thread(monitorDevices, std::ref(running));
    Context ContextInstance(key_states, key_mutex);
    std::thread Ray_r_thread(RasterizeLoop, std::ref(ContextInstance), std::ref(running));
    std::thread Ray_u_thread(UpdateLoop, std::ref(ContextInstance), std::ref(running));

    int rows, cols;
    while (running) {
        ContextInstance.Render();
    }
    // Chiudi curses e ripristina il terminal
    Ray_r_thread.join();
    Ray_u_thread.join();
    monitor_thread.join();

    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}