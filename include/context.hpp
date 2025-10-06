#pragma once
#include <ncurses.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "buffer2d.hpp"
#include "doublebuffer.hpp"
#include "milkman.hpp"
#include "scene.hpp"
class Context {
   public:
    MilkMan<int> pino;
    MilkMan<Scene> SceneInstance;
    double PixelRatio = 2.06;                    // 2.06
    const std::array<bool, 256>& InputStateRef;  // it'd be nice to wrap them toghether?
    std::array<bool, 256> InputState;
    std::mutex& InputStateMutex;

    std::mutex& OutputStateMutex;

    DoubleBuffer<Buffer2D<struct Pixel>> PixelBuffer;

    point2 Project(const point3& obj, const Camera& CameraInstance);
    void Update();
    std::size_t rows, cols;

    bool debug = true;
    std::mutex& bufferMutex;
    void GetDimensions();

    void Render();

    Context(const std::array<bool, 256>& key_states, std::mutex& key_mutex,
            std::mutex& buffer_mutex);
    ~Context();
};

void UpdateLoop(Context& ref, std::atomic<bool>& running);