#include "scene.hpp"
Scene::Scene()
    : CameraInstance({0, 0, 7}, {0, 0}),
      Triangles({{{10, 0, 10}, {10, 5, 5}, {10, -5, 5}},
                 {{8, -2, 0}, {4, -2, 0}, {6, 0, 6}},
                 {{8, +2, 0}, {4, +2, 0}, {6, 0, 6}},
                 {{8, -2, 0}, {8, +2, 0}, {6, 0, 6}},
                 {{4, -2, 0}, {4, +2, 0}, {6, 0, 6}}}) {
    Points.push_back({0, 0, 0});
    for (int _i = 1; _i <= 20; _i++) {
        double i = (double)_i;
        Points.push_back({0, 0, i});
        Points.push_back({0, i, 0});
        Points.push_back({i, 0, 0});

        Points.push_back({0, 0, -i});
        Points.push_back({0, -i, 0});
        Points.push_back({-i, 0, 0});
    }
    for (int _j = 1; _j <= 20; _j++) {
        for (int _i = 1; _i <= 20; _i++) {
            double i = (double)_i, j = (double)_j;
            Points.push_back({i, j, 0});  //  floor
            Points.push_back({-i, j, 0});
            Points.push_back({i, -j, 0});
            Points.push_back({-i, -j, 0});

            Points.push_back({0, i, j});  // wall
        }
        Points.push_back({10, 5, 5});
        Points.push_back({10, -5, 5});
        Points.push_back({10, 0, 10});
    }
}