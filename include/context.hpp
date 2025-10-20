#pragma once
#include <ncurses.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "buffer2d.hpp"
#include "doublebuffer.hpp"
#include "input_handler.hpp"
#include "milkman.hpp"
#include "scene.hpp"

constexpr int UPS = 24;
constexpr std::chrono::milliseconds MS_PER_UPDATE(
    1000 /
    UPS);  // if you keep them contexpr and bring them inside Scene you'll need static as well

class Context {
    std::thread UpdateThread;
    std::thread RasterizeThread;
    std::thread MonitorThread;
    std::atomic<bool>& Running;

    MilkMan<Scene> SceneInstance;
    double PixelRatio;  // 2.06

    InputHandler InputHandlerInstance;

    DoubleBuffer<Buffer2D<struct Pixel>> PixelBuffer;

    std::size_t rows, cols;

    bool debug;
    void GetDimensions();

    void Update();
    void Rasterize();

    void UpdateLoop();
    void RasterizeLoop();

   public:
    Context(std::atomic<bool>& running);
    ~Context();
    void Render();
};
