#pragma once
struct direction3;
struct point3 {
    double X;
    double Y;
    double Z;

    point3(double a, double b, double c);

    point3 operator*(const double l) const;
    point3 operator/(const double l) const;

    point3(const direction3& d, const double len);

    double operator*(const point3& other) const;

    double Length() const;

    double Theta() const;  // Angle on the X-Y plane.
    double Phi() const;

    direction3 Direction() const;

    point3(const point3&);
    point3(point3&&);
    point3();
    point3& operator=(const point3&);
    point3& operator=(point3&&);
    point3 Normalize() const;
    point3 operator+(const point3& other) const;
    point3 operator-(const point3& other) const;
    point3 EvenPart(const point3& other) const;
    point3 OddPart(const point3& other) const;
    point3 CrossProduct(const point3& other) const;
    double AngleFrom(const point3& other) const;
    point3 Rotate(const point3& o, double t) const;
    point3 RotatePhi90Up();
    point3 RotateTheta90YX();
};