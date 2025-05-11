#include "scene.hpp"
Scene::Scene(const std::array<bool, 256>& key_states, std::mutex& key_mutex,
             std::mutex& buffer_mutex)
    : InputStateRef(key_states),
      InputStateMutex(key_mutex),
      Pix0(std::vector<std::vector<Pixel>>(0, std::vector<Pixel>(0))),
      Pix1(std::vector<std::vector<Pixel>>(0, std::vector<Pixel>(0))),
      Pixels(Pix0),
      Rixels(Pix1),
      OutputStateMutex(buffer_mutex),
      CameraInstance({0, 0, 7}, {0, 0}),
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

point2 Scene::Project(const point3& obj) {
    point3 diff = obj - CameraInstance.Position;

    point3 up = CameraInstance.Direction.UnitVector().RotatePhi90Up();
    point3 right = CameraInstance.Direction.UnitVector().RotateTheta90YX();
    point3 pro = up.OddPart(diff);
    point3 bro = right.OddPart(diff);
    // return { wrapToPi(diff.Theta() - CameraInstance.Direction.Theta)*cos(diff.Phi()),
    // ((diff.Phi() - CameraInstance.Direction.Phi)) };

    return {atan2(up * CameraInstance.Direction.UnitVector().CrossProduct(pro.Normalize()),
                  CameraInstance.Direction.UnitVector() * pro.Normalize()),
            atan2(right * bro.Normalize().CrossProduct(CameraInstance.Direction.UnitVector()),
                  CameraInstance.Direction.UnitVector() * bro.Normalize())};
}
void Scene::Update() {
    std::unique_lock<std::mutex> pock(InputStateMutex);
    InputState = InputStateRef;
    pock.unlock();

    if (InputState[103]) {
        if (CameraInstance.Direction.Phi <= M_PI_2) CameraInstance.Direction.Phi += 0.1;
    }
    if (InputState[108]) {
        if (CameraInstance.Direction.Phi >= -M_PI_2) CameraInstance.Direction.Phi -= 0.1;
    }
    if (InputState[105]) {
        CameraInstance.Direction.Theta -= 0.15;
    }
    if (InputState[106]) {
        CameraInstance.Direction.Theta += 0.15;
    }
    if (InputState[30]) {
        CameraInstance.Position.Y -= step * cos(CameraInstance.Direction.Theta);
        CameraInstance.Position.X += step * sin(CameraInstance.Direction.Theta);
    }
    if (InputState[32]) {
        CameraInstance.Position.Y += step * cos(CameraInstance.Direction.Theta);
        CameraInstance.Position.X -= step * sin(CameraInstance.Direction.Theta);
    }
    if (InputState[17]) {
        CameraInstance.Position.X += step * cos(CameraInstance.Direction.Theta);
        CameraInstance.Position.Y += step * sin(CameraInstance.Direction.Theta);
    }
    if (InputState[31]) {
        CameraInstance.Position.X -= step * cos(CameraInstance.Direction.Theta);
        CameraInstance.Position.Y -= step * sin(CameraInstance.Direction.Theta);
    }
    if (InputState[57]) {
        if (!jumping) {
            vz = 0.6;
            g = -0.03;
            jumping = true;
        }
    }
    CameraInstance.Position.Z += vz;
    vz += g;
    if (CameraInstance.Position.Z < 7) {
        CameraInstance.Position.Z = 7;
        vz = 0;
        g = 0;
        jumping = false;
    }
    if (InputState[52]) {
    }
    if (InputState[53]) {
    }
    if (InputState[23]) {
        debug = !debug;
    }
    // dato un pixel a x-y
    // se ne calcola il vettore unitario corrispondente
    int rows = Pixels.size();
    int cols = Pixels[0].size();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Pixels[i][j].Char = ' ';
            Pixels[i][j].Distance = NAN;
        }
    }
    for (int i = 0; i < Points.size(); i++) {
        double distance = (Points[i] - CameraInstance.Position).Length();
        point2 projection = Project(Points[i]);
        int yy = (int)round(((projection.X / PixelRatio) + 0.5) * cols);
        int xx = (int)round(((-projection.Y) + 0.5) * rows);
        if ((xx >= 0) && (yy >= 0) && (xx < rows) && (yy < cols)) {
            if ((distance > 0) && ((Pixels[xx][yy].Distance != Pixels[xx][yy].Distance) ||
                                   (Pixels[xx][yy].Distance > distance))) {
                Pixels[xx][yy].Distance = distance;
                Pixels[xx][yy].Char = '.';
                Pixels[xx][yy].Colour = 7;
            }
        }
    }
    for (int i = 0; i < Pixels.size(); i++) {
        for (int j = 0; j < Pixels[0].size(); j++) {
            // point3 d = ( CameraInstance.Direction + direction3( (( (double)j / cols) -
            // 0.5)*2.06,  - (( (double)i / rows) - 0.5  ) )).UnitVector();
            point3 u = CameraInstance.Direction.UnitVector().RotatePhi90Up();
            point3 r = CameraInstance.Direction.UnitVector().RotateTheta90YX();
            point3 l = u.Rotate(r, ((((double)j / cols) - 0.5) * PixelRatio));
            point3 h = r.Rotate(u, (((double)i / rows) - 0.5));
            // d = u.CrossProduct(d).Rotate(d, (( (double)i / rows) - 0.5)*cos((( (double)j /
            // cols) - 0.5)*PixelRatio)*(1-cos(( (double)i / rows) - 0.5)));
            point3 d = l.CrossProduct(h).Normalize();
            /*point3 d = CameraInstance.Direction.UnitVector().RotatePhi90Up().Rotate(
                CameraInstance.Direction.UnitVector().RotateTheta90YX().Rotate(
                        CameraInstance.Direction.UnitVector(),
                        tan(( (double)i / rows) - 0.5) ), ((( (double)j / cols) - 0.5)*2.06));*/
            // point3 d = ( CameraInstance.Direction + direction3( (( (double)j / cols) - 0.5)
            // *2.06, - (( (double)i / rows) - 0.5 ) ) ).UnitVector(); si risolve per tutti i
            // piani/triangoli

            for (int f = 0; f < Triangles.size(); f++) {
                point3 B = Triangles[f].b - Triangles[f].a;
                point3 C = Triangles[f].c - Triangles[f].a;
                const point3& P0 = Triangles[f].a;
                double det = B.Z * C.Y * d.X - B.Y * C.Z * d.X - B.Z * C.X * d.Y + B.X * C.Z * d.Y +
                             B.Y * C.X * d.Z - B.X * C.Y * d.Z;
                point3 k = CameraInstance.Position - P0;
                double t =
                    point3(-B.Z * C.Y + B.Y * C.Z, B.Z * C.X - B.X * C.Z, -B.Y * C.X + B.X * C.Y) *
                    k / det;
                double u =
                    point3(C.Z * d.Y - C.Y * d.Z, -C.Z * d.X + C.X * d.Z, C.Y * d.X - C.X * d.Y) *
                    k / det;
                double v =
                    point3(-B.Z * d.Y + B.Y * d.Z, B.Z * d.X - B.X * d.Z, -B.Y * d.X + B.X * d.Y) *
                    k / det;
                // si salva il carattere corrispondente alla distanza minore
                if ((t > 0) && (u >= 0) && (v >= 0) && (u <= 1) && (v <= 1) && (u + v <= 1)) {
                    if ((Pixels[i][j].Distance > t) ||
                        (Pixels[i][j].Distance != Pixels[i][j].Distance)) {
                        Pixels[i][j].Char = '#';
                        Pixels[i][j].Colour = f + 1;
                        Pixels[i][j].Distance = t;
                    }
                }
            }
        }
    }
    std::unique_lock<std::mutex> mock(OutputStateMutex);
    Rixels = Pixels;
    Pixels = &Pixels == &Pix0 ? Pix1 : Pix0;
    mock.unlock();
}