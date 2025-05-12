#pragma once
#include "direction3.hpp"
#include "point3.hpp"

class Camera {
   public:
    point3 Position;
    direction3 Direction;
    point3 Speed;
    Camera(point3 p, direction3 d);
};