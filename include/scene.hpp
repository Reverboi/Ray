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

class Scene {
   public:
    Camera CameraInstance;

    bool jumping = false;
    double step = 0.6;
    double g = 0, vz = 0;

    std::vector<triangle3> Triangles;
    std::vector<point3> Points;

    Scene();
};