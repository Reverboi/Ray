#include <atomic>
#include <thread>

#include "context.hpp"
#include "input_handler.hpp"

extern std::atomic<bool> running;
std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false, std::memory_order_relaxed);
}

constexpr std::chrono::milliseconds MS_PER_RENDER(60);

int main() {
    signal(SIGINT, signalHandler);
    std::thread monitor_thread(monitorDevices, std::ref(running));
    std::mutex buffer_mutex;
    Context ContextInstance(key_states, key_mutex, buffer_mutex);
    std::thread Ray_thread(UpdateLoop, std::ref(ContextInstance), std::ref(running));
    /*
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    */
    int rows, cols;
    while (running) {
        // frameStart = std::chrono::high_resolution_clock::now();

        ContextInstance.Render();

        std::this_thread::sleep_for(MS_PER_RENDER);
    }
    // Chiudi curses e ripristina il terminal
    Ray_thread.join();
    monitor_thread.join();

    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}