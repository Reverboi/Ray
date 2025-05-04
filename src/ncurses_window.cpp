#include "ncurses_window.hpp"

std::vector<std::vector<struct Pixel>> load_buffer;
std::mutex buffer_mutex;

int mFPS = 28; // do these two really need to be consdtexpr? what if i want  to be able to change fps mid-game for any reasson
std::chrono::milliseconds mMS_PER_FRAME(1000 / mFPS);

void GetDimensions(int& rows, int& cols){
    getmaxyx(stdscr, rows, cols);
}

void NcursesExit(){
    endwin();
}
void NcursesInit() {
    initscr();
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
    refresh();
}

void Render(){
        clear();
        int rows;
        int cols;
        GetDimensions(rows, cols);
        std::unique_lock<std::mutex> lock(buffer_mutex);
        for (int y = 0; y < rows; ++y){
            for (int x = 0; x < cols; ++x){   
                attron(COLOR_PAIR(load_buffer[y][x].Colour));
                mvaddch( y, x, load_buffer[y][x].Char ) ;
                attroff(COLOR_PAIR(load_buffer[y][x].Colour));
            }
        }
        lock.unlock();
        std::this_thread::sleep_for(mMS_PER_FRAME);
}