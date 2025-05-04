#include "ray.hpp"

point3::point3(const direction3& d, const double len) : point3(d.UnitVector() * len) {}

direction3 point3::Direction() const { return { Theta(), Phi() }; }
direction3::direction3(const point3& a) : direction3(a.Direction()) {}

double wrapToPi(double angle) {
    angle = std::fmod(angle + M_PI, 2 * M_PI);
    if (angle < 0) angle += 2 * M_PI;
    return angle - M_PI;
}