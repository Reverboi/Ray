#pragma once
struct point3;
struct direction3 {
    double Theta;
    double Phi;

    direction3(const point3& p);
    direction3(double a, double b) : Theta(a), Phi(b) {}

    double X() const;
    double Y() const;
    double Z() const;

    point3 UnitVector() const;
    direction3(const direction3&);
    direction3(direction3&&);
    direction3();
    direction3& operator=(const direction3&);
    direction3& operator=(direction3&&);
};