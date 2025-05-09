#include <atomic>
#include <thread>

#include "input_handler.hpp"
#include "ncurses_window.hpp"
#include "scene.hpp"

extern std::atomic<bool> running;
std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false, std::memory_order_relaxed);
}

constexpr std::chrono::milliseconds MS_PER_RENDER(60);
int main() {
    signal(SIGINT, signalHandler);
    std::thread monitor_thread(monitorDevices, std::ref(running));
    NcursesInit();
    int rows, cols;
    GetDimensions(rows, cols);

    Scene SceneInstance(key_states, key_mutex, buffer_mutex, cols, rows);
    std::thread Ray_thread(UpdateLoop, std::ref(SceneInstance), std::ref(running));

    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        GetDimensions(rows, cols);
        if ((rows != SceneInstance.Pixels.size()) || (cols != SceneInstance.Pixels[0].size())) {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            SceneInstance.Pixels = std::vector<std::vector<Pixel>>(rows, std::vector<Pixel>(cols));
            SceneInstance.Rixels = std::vector<std::vector<Pixel>>(rows, std::vector<Pixel>(cols));
        }

        SceneInstance.Render();

        std::this_thread::sleep_for(MS_PER_RENDER);
    }
    // Chiudi curses e ripristina il terminale
    NcursesExit();
    Ray_thread.join();
    monitor_thread.join();
    // ncurses_thread.join();
    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}