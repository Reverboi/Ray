#pragma once
#include <ncurses.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "buffer2d.hpp"
#include "doublebuffer.hpp"
#include "logger.hpp"
#include "milkman.hpp"
#include "scene.hpp"

constexpr int UPS = 24;
constexpr std::chrono::milliseconds MS_PER_UPDATE(
    1000 /
    UPS);  // if you keep them contexpr and bring them inside Scene you'll need static as well

class Context {
   public:
    MilkMan<Scene> SceneInstance;
    double PixelRatio = 2.06;                    // 2.06
    const std::array<bool, 256>& InputStateRef;  // it'd be nice to wrap them toghether?
    std::array<bool, 256> InputState;
    std::mutex& InputStateMutex;

    DoubleBuffer<Buffer2D<struct Pixel>> PixelBuffer;

    std::size_t rows, cols;

    bool debug = true;
    void GetDimensions();

    void Render();
    void Update();
    void Rasterize();
    Context(const std::array<bool, 256>& key_states, std::mutex& key_mutex);
    ~Context();
};

void UpdateLoop(Context& ref, std::atomic<bool>& running);
void RenderLoop(Context& ref, std::atomic<bool>& running);
void RasterizeLoop(Context& ref, std::atomic<bool>& running);