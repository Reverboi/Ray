#pragma once
#include <ncurses.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

#include "camera.hpp"
#include "direction3.hpp"
#include "pixel.hpp"
#include "point2.hpp"
#include "point3.hpp"
#include "triangle3.hpp"

constexpr int UPS = 16;
constexpr std::chrono::milliseconds MS_PER_UPDATE(
    1000 /
    UPS);  // if you keep them contexpr and bring them inside Scene you'll need static as well

class Scene {
   public:
    Camera CameraInstance;

    double PixelRatio = 2.06;  // 2.06
    bool jumping = false;
    bool debug = true;
    double step = 0.6;
    double g = 0, vz = 0;
    int targetMilliseconds = 16;  // milliseconds

    std::vector<triangle3> Triangles;
    std::vector<point3> Points;

    const std::array<bool, 256>& InputStateRef;  // it'd be nice to wrap them toghether?
    std::array<bool, 256> InputState;
    std::mutex& InputStateMutex;

    std::mutex& OutputStateMutex;

    std::vector<std::vector<struct Pixel>>& Pixels;
    std::vector<std::vector<struct Pixel>>& Rixels;
    std::vector<std::vector<struct Pixel>> Pix0;
    std::vector<std::vector<struct Pixel>> Pix1;
    Scene(const std::array<bool, 256>& key_states, std::mutex& key_mutex, std::mutex& buffer_mutex);
    point2 Project(const point3& obj);
    void Update();
};