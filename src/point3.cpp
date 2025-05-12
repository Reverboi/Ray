#include "point3.hpp"

#include <cmath>

#include "direction3.hpp"

point3::point3(double a = 0, double b = 0, double c = 0) : X(a), Y(b), Z(c) {}
double point3::operator*(const point3& other) const {
    return X * other.X + Y * other.Y + Z * other.Z;
}
point3 point3::operator*(const double l) const {
    return {l * X, l * Y, l * Z};
}
point3 point3::operator/(const double l) const {
    return {X / l, Y / l, Z / l};
}

double point3::Length() const {
    return std::sqrt(*this * *this);
}

double point3::Theta() const {
    return std::atan2(Y, X);
}  // Angle on the X-Y plane.
double point3::Phi() const {
    return std::atan2(Z, std::sqrt(X * X + Y * Y));
}  // Angle from the X-Y plane.

point3::point3(const direction3& d, const double len) : point3(d.UnitVector() * len) {}

direction3 point3::Direction() const {
    return {Theta(), Phi()};
}

point3::point3(const point3&) = default;
point3::point3(point3&&) = default;
point3::point3() : X(0.0), Y(0.0), Z(0.0){};

point3& point3::operator=(const point3&) = default;
point3& point3::operator=(point3&&) = default;

point3 point3::Normalize() const {
    return *this / Length();
}

point3 point3::operator+(const point3& other) const {
    return {X + other.X, Y + other.Y, Z + other.Z};
}
point3& point3::operator+=(const point3& other) {
    *this = *this + other;
    return *this;
}
point3& point3::operator-=(const point3& other) {
    *this = *this - other;
    return *this;
}
point3& point3::operator*=(const double p) {
    *this = *this * p;
    return *this;
}
point3& point3::operator/=(const double p) {
    *this = *this / p;
    return *this;
}
point3 point3::operator-(const point3& other) const {
    return {X - other.X, Y - other.Y, Z - other.Z};
}
point3 point3::EvenPart(const point3& other) const {
    return (*this) * (*this * other) / (*this * *this);
}
point3 point3::OddPart(const point3& other) const {
    return other - EvenPart(other);
}
point3 point3::CrossProduct(const point3& other) const {
    return {Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X};
}
double point3::AngleFrom(const point3& other) const {  // bad
    point3 a = Normalize();
    point3 b = other.Normalize();
    return atan2(a.CrossProduct(b).Length(), a * b);
}
point3 point3::Rotate(const point3& o, double t) const {
    point3 d = Normalize();
    return {(cos(t) + d.X * d.X * (1 - cos(t))) * o.X +
                (d.X * d.Y * (1 - cos(t)) - d.Z * sin(t)) * o.Y +
                (d.X * d.Z * (1 - cos(t)) + d.Y * sin(t)) * o.Z,
            (d.Y * d.X * (1 - cos(t)) + d.Z * sin(t)) * o.X +
                (cos(t) + d.Y * d.Y * (1 - cos(t))) * o.Y +
                (d.Y * d.Z * (1 - cos(t)) - d.X * sin(t)) * o.Z,
            (d.Z * d.X * (1 - cos(t)) - d.Y * sin(t)) * o.X +
                (d.Z * d.Y * (1 - cos(t)) + d.X * sin(t)) * o.Y +
                (cos(t) + d.Z * d.Z * (1 - cos(t))) * o.Z};
}
point3 point3::RotatePhi90Up() {
    return {-Z * X / sqrt(X * X + Y * Y), -Z * Y / sqrt(X * X + Y * Y), sqrt(X * X + Y * Y)};
}
point3 point3::RotateTheta90YX() {
    return RotatePhi90Up().CrossProduct(*this);
}  // does not preserve module if!=1