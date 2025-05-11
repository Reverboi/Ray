#include "context.hpp"

void Context::GetDimensions() {
    getmaxyx(stdscr, rows, cols);
}

Context::~Context() {
    endwin();
}

Context::Context(const std::array<bool, 256>& key_states, std::mutex& key_mutex,
                 std::mutex& buffer_mutex)
    : Scn(key_states, key_mutex, buffer_mutex), bufferMutex(buffer_mutex) {
    initscr();
    start_color();
    GetDimensions();
    Scn.Pixels = std::vector<std::vector<Pixel>>(rows, std::vector<Pixel>(cols));
    Scn.Rixels = std::vector<std::vector<Pixel>>(rows, std::vector<Pixel>(cols));
    //  Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    cbreak();  // Disabilita il buffering dell'input
    noecho();
    // nodelay(stdscr, TRUE); makes getch() impatient
    refresh();
}

void Context::Render() {
    clear();
    std::unique_lock<std::mutex> lock(bufferMutex);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            attron(COLOR_PAIR(Scn.Rixels[y][x].Colour));
            mvaddch(y, x, Scn.Rixels[y][x].Char);
            attroff(COLOR_PAIR(Scn.Rixels[y][x].Colour));
        }
    }

    int s = 0;
    if (Scn.debug) {
        point3 diff = point3(10, 0, 10) - Scn.CameraInstance.Position;
        point3 up = Scn.CameraInstance.Direction.UnitVector().RotatePhi90Up();
        point3 right = Scn.CameraInstance.Direction.UnitVector().RotateTheta90YX();
        point3 pro = up.OddPart(diff);
        point3 bro = right.OddPart(diff);
        mvprintw(s++, 0, "theta---: %.2f phi---: %.2f", Scn.CameraInstance.Direction.Theta,
                 Scn.CameraInstance.Direction.Phi);
        mvprintw(s++, 0, "thetapro: %.2f phipro: %.2f", pro.Theta(), pro.Phi());
        mvprintw(s++, 0, "thetabro: %.2f phibro: %.2f", bro.Theta(), bro.Phi());
        mvprintw(s++, 0, "diff-len: %.2f diff-norm-len: %.2f", diff.Length(),
                 diff.Normalize().Length());
        mvprintw(
            s++, 0, "thetadif: %.2f phidif: %.2f",
            atan2(up * Scn.CameraInstance.Direction.UnitVector().CrossProduct(pro.Normalize()),
                  Scn.CameraInstance.Direction.UnitVector() * pro.Normalize()),
            atan2(right * Scn.CameraInstance.Direction.UnitVector().CrossProduct(bro.Normalize()),
                  Scn.CameraInstance.Direction.UnitVector() * bro.Normalize()));
        mvprintw(s++, 0, "X: %.2f Y: %.2f Z: %.2f", Scn.CameraInstance.Position.X,
                 Scn.CameraInstance.Position.Y, Scn.CameraInstance.Position.Z);
        mvprintw(s++, 0, "cols = %d, rows = %d, c/r = %.2f", cols, rows, (float)cols / rows);
        mvprintw(s++, 0, "frame time = %d teomaxFPS = %.2f", Scn.targetMilliseconds,
                 1000.0 / Scn.targetMilliseconds);
        mvprintw(s++, 0, "looop time = %ld fixed FPS = %.2f", MS_PER_UPDATE.count(),
                 1000.0 / MS_PER_UPDATE.count());
    }
    lock.unlock();
    refresh();  // Aggiorna lo schermo
}

void UpdateLoop(Context& ref, std::atomic<bool>& running) {
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        ref.GetDimensions();
        if ((ref.rows != ref.Scn.Pixels.size()) || (ref.cols != ref.Scn.Pixels[0].size())) {
            std::lock_guard<std::mutex> lock(ref.bufferMutex);
            ref.Scn.Pixels =
                std::vector<std::vector<Pixel>>(ref.rows, std::vector<Pixel>(ref.cols));
            ref.Scn.Rixels =
                std::vector<std::vector<Pixel>>(ref.rows, std::vector<Pixel>(ref.cols));
        }

        ref.Scn.Update();
        frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (frameDuration < MS_PER_UPDATE) {
            std::this_thread::sleep_for(MS_PER_UPDATE - frameDuration);
        }
    }
}