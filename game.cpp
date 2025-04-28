#include <thread>
#include "ray.cpp"
#include "input_handler.cpp"
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false,std::memory_order_relaxed);
}

int main() {
    signal(SIGINT, signalHandler);

    std::thread monitor_thread(monitorDevices, std::ref(running));
    // Inizializzazione di curses
    initscr();     // Inizializza il terminale in modalità curses
    start_color(); // Enable color functionality

    // Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    cbreak();              // Disabilita il buffering dell'input
    noecho();
    // nodelay(stdscr, TRUE); makes getch() impatient
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    Scene SceneInstance(key_states, key_mutex, cols, rows);
    refresh(); // Aggiorna lo schermo per visualizzare il testo
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        getmaxyx(stdscr, rows, cols);
        if ((rows!=SceneInstance.Pixels.size())||(cols!=SceneInstance.Pixels[0].size())){
            SceneInstance.Pixels = std::vector<std::vector<Pixel>>( rows, std::vector<Pixel>( cols ) );
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
    endwin();

    monitor_thread.join();

    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}