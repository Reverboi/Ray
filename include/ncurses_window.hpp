#pragma once
#include <ncurses.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "pixel.hpp"
extern std::vector<std::vector<struct Pixel>> load_buffer;
extern std::mutex buffer_mutex;
void GetDimensions(int& rows, int& cols);

void NcursesInit();
void NcursesExit();
void Render();