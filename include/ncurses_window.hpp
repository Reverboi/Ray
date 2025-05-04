#pragma once
#include <ncurses.h>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include "pixel.hpp"
extern std::vector<std::vector<struct Pixel>> load_buffer;
extern std::mutex buffer_mutex;

void GetDimensions(int& rows, int& cols);

void NcursesInit();
void NcursesExit();
extern int mFPS; // do these two really need to be consdtexpr? what if i want  to be able to change fps mid-game for any reasson
extern std::chrono::milliseconds mMS_PER_FRAME;
void Render();