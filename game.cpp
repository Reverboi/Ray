#include <thread>
#include "ray.cpp"
#include "input_handler.cpp"

void displayLoop() {
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
    Scene SceneInstance(cols, rows);
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
        std::unique_lock<std::mutex> lock(key_mutex);
        
        if (!key_states.empty()) { 
            if (key_states[103].is){
                if ( SceneInstance.CameraInstance.Direction.Phi <= M_PI_2 )
                    SceneInstance.CameraInstance.Direction.Phi += 0.1;
            }
            if (key_states[108].is){
                if ( SceneInstance.CameraInstance.Direction.Phi >= - M_PI_2 )
                SceneInstance.CameraInstance.Direction.Phi -= 0.1;
            }
            if (key_states[105].is){
                SceneInstance.CameraInstance.Direction.Theta -= 0.15;
            }
            if (key_states[106].is){
                SceneInstance.CameraInstance.Direction.Theta += 0.15;
            }
            if (key_states[30].is){
                SceneInstance.CameraInstance.Position.Y -= SceneInstance.step * cos(SceneInstance.CameraInstance.Direction.Theta);
                SceneInstance.CameraInstance.Position.X += SceneInstance.step * sin(SceneInstance.CameraInstance.Direction.Theta);
            }
            if (key_states[32].is){
                SceneInstance.CameraInstance.Position.Y += SceneInstance.step * cos(SceneInstance.CameraInstance.Direction.Theta);
                SceneInstance.CameraInstance.Position.X -= SceneInstance.step * sin(SceneInstance.CameraInstance.Direction.Theta);
            }
            if (key_states[17].is){
                SceneInstance.CameraInstance.Position.X += SceneInstance.step * cos(SceneInstance.CameraInstance.Direction.Theta);
                SceneInstance.CameraInstance.Position.Y += SceneInstance.step * sin(SceneInstance.CameraInstance.Direction.Theta);
            }
            if (key_states[31].is){
                SceneInstance.CameraInstance.Position.X -= SceneInstance.step * cos(SceneInstance.CameraInstance.Direction.Theta);
                SceneInstance.CameraInstance.Position.Y -= SceneInstance.step * sin(SceneInstance.CameraInstance.Direction.Theta);
            }
            if (key_states[57].is){
                if (!SceneInstance.jumping){
                    SceneInstance.vz = 0.6;
                    SceneInstance.g = -0.03;
                    SceneInstance.jumping = true;
                }
            }
            SceneInstance.CameraInstance.Position.Z += SceneInstance.vz;
            SceneInstance.vz +=SceneInstance.g;
            if(SceneInstance.CameraInstance.Position.Z<7){
                SceneInstance.CameraInstance.Position.Z = 7;
                SceneInstance.vz = 0;
                SceneInstance.g = 0;
                SceneInstance.jumping = false;
            }
            if (key_states[52].is){
            }
            if (key_states[53].is){
            }
            if (key_states[23].is){
                SceneInstance.debug = !SceneInstance.debug;
            }
        }
        SceneInstance.render();
        lock.unlock();
        frameEnd = std::chrono::high_resolution_clock::now();
        frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        SceneInstance.frameDuration = frameDuration.count();
        if (frameDuration < MS_PER_FRAME) {
            std::this_thread::sleep_for(MS_PER_FRAME - frameDuration);
        }
    }
    // Chiudi curses e ripristina il terminale
    endwin();
}

int main() {
    signal(SIGINT, signalHandler);

    std::thread monitor_thread(monitorDevices);
    displayLoop();

    monitor_thread.join();

    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}