#include "camera.hpp"

Camera::Camera(point3 p, direction3 d) : Position(p), Direction(d), Speed(){};

point2 Camera::Project(const point3& obj) const {
    point3 diff = obj - Position;

    point3 up = Direction.UnitVector().RotatePhi90Up();
    point3 right = Direction.UnitVector().RotateTheta90YX();
    point3 pro = up.OddPart(diff);
    point3 bro = right.OddPart(diff);
    // return { wrapToPi(diff.Theta() - CameraInstance.Direction.Theta)*cos(diff.Phi()),
    // ((diff.Phi() - CameraInstance.Direction.Phi)) };

    return {atan2(up * Direction.UnitVector().CrossProduct(pro.Normalize()),
                  Direction.UnitVector() * pro.Normalize()),
            atan2(right * bro.Normalize().CrossProduct(Direction.UnitVector()),
                  Direction.UnitVector() * bro.Normalize())};
}
