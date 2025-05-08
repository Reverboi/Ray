#pragma once
#include "point3.hpp"
struct triangle3 {
    point3 a, b, c;
    int Colour = 1;
    triangle3(const point3& a, const point3& b, const point3& c);
    triangle3();
};