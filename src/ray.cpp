#include "ray.hpp"

point3::point3(const direction3& d, const double len) : point3(d.UnitVector() * len) {}

direction3 point3::Direction() const {
    return {Theta(), Phi()};
}
direction3::direction3(const point3& a) : direction3(a.Direction()) {}

void UpdateLoop(Scene& ref, std::atomic<bool>& running) {
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();
        ref.Update();
        frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (frameDuration < MS_PER_UPDATE) {
            std::this_thread::sleep_for(MS_PER_UPDATE - frameDuration);
        }
    }
}