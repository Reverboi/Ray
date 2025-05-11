#pragma once
#include <ncurses.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "scene.hpp"
class Context {
   public:
    Scene Scn;
    int rows, cols;

    std::mutex& bufferMutex;
    void GetDimensions();

    void Render();

    Context(const std::array<bool, 256>& key_states, std::mutex& key_mutex,
            std::mutex& buffer_mutex);
    ~Context();
};

void UpdateLoop(Context& ref, std::atomic<bool>& running);