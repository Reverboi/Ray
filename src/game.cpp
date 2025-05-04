#include <thread>
#include "ray.hpp"
#include "input_handler.hpp"
#include "ncurses_window.hpp"
#include <atomic>

extern std::atomic<bool> running;
std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false,std::memory_order_relaxed);
}

int main() {
    signal(SIGINT, signalHandler);
    std::thread monitor_thread(monitorDevices, std::ref(running));
    NcursesInit();
    int rows, cols;
    GetDimensions(rows, cols);

    Scene SceneInstance(key_states, key_mutex, load_buffer, buffer_mutex, cols, rows);

    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    //std::thread ncurses_thread(RenderThread, std::ref(running));
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        GetDimensions(rows, cols);
        if ((rows!=SceneInstance.Pixels.size())||(cols!=SceneInstance.Pixels[0].size())){
            SceneInstance.Pixels = std::vector<std::vector<Pixel>>( rows, std::vector<Pixel>( cols ) );
            std::unique_lock<std::mutex> lock(buffer_mutex);
            load_buffer = std::vector<std::vector<Pixel>>(rows, std::vector<Pixel>(cols));
            lock.unlock();
        }
        
        SceneInstance.Render();
        frameEnd = std::chrono::high_resolution_clock::now();
        frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        SceneInstance.frameDuration = frameDuration.count();
        if (frameDuration < MS_PER_FRAME) {
            std::this_thread::sleep_for(MS_PER_FRAME - frameDuration);
        }
    }
    // Chiudi curses e ripristina il terminale
    NcursesExit();

    monitor_thread.join();
    //ncurses_thread.join();
    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}