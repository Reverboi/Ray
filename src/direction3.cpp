#include "direction3.hpp"

#include <cmath>

#include "point3.hpp"
double direction3::X() const {
    return std::cos(Theta) * std::cos(Phi);
}
double direction3::Y() const {
    return std::sin(Theta) * std::cos(Phi);
}
double direction3::Z() const {
    return std::sin(Phi);
}

point3 direction3::UnitVector() const {
    return {X(), Y(), Z()};
}
direction3::direction3(const direction3&) = default;
direction3::direction3(direction3&&) = default;
direction3::direction3() = default;
direction3::direction3(const point3& a) : direction3(a.Direction()) {}
