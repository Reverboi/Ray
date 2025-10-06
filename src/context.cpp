#include "context.hpp"

void Context::GetDimensions() {
    getmaxyx(stdscr, rows, cols);
}

Context::~Context() {
    endwin();
}

Context::Context(const std::array<bool, 256>& key_states, std::mutex& key_mutex,
                 std::mutex& buffer_mutex)
    : InputStateRef(key_states),
      InputStateMutex(key_mutex),
      OutputStateMutex(buffer_mutex),
      SceneInstance(),
      PixelBuffer(rows, cols),
      bufferMutex(buffer_mutex) {
    initscr();
    start_color();
    GetDimensions();
    Buffer2D<Pixel> bu(rows, cols);
    PixelBuffer = DoubleBuffer<Buffer2D<Pixel>>(rows, cols);
    //  Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    cbreak();  // Disabilita il buffering dell'input
    noecho();
    curs_set(0);
    // nodelay(stdscr, TRUE); makes getch() impatient
    refresh();
}

void Context::Render() {
    clear();
    std::unique_lock<std::mutex> lock(bufferMutex);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            attron(COLOR_PAIR(Rixels[y][x].Colour));
            mvaddch(y, x, Rixels[y][x].Char);
            attroff(COLOR_PAIR(Rixels[y][x].Colour));
        }
    }

    int s = 0;
    if (debug) {
        point3 diff = point3(10, 0, 10) - SceneInstance.CameraInstance.Position;
        point3 up = SceneInstance.CameraInstance.Direction.UnitVector().RotatePhi90Up();
        point3 right = SceneInstance.CameraInstance.Direction.UnitVector().RotateTheta90YX();
        point3 pro = up.OddPart(diff);
        point3 bro = right.OddPart(diff);
        mvprintw(s++, 0, "theta---: %.2f phi---: %.2f",
                 SceneInstance.CameraInstance.Direction.Theta,
                 SceneInstance.CameraInstance.Direction.Phi);
        mvprintw(s++, 0, "thetapro: %.2f phipro: %.2f", pro.Theta(), pro.Phi());
        mvprintw(s++, 0, "thetabro: %.2f phibro: %.2f", bro.Theta(), bro.Phi());
        mvprintw(s++, 0, "diff-len: %.2f diff-norm-len: %.2f", diff.Length(),
                 diff.Normalize().Length());
        mvprintw(s++, 0, "thetadif: %.2f phidif: %.2f",
                 atan2(up * SceneInstance.CameraInstance.Direction.UnitVector().CrossProduct(
                                pro.Normalize()),
                       SceneInstance.CameraInstance.Direction.UnitVector() * pro.Normalize()),
                 atan2(right * SceneInstance.CameraInstance.Direction.UnitVector().CrossProduct(
                                   bro.Normalize()),
                       SceneInstance.CameraInstance.Direction.UnitVector() * bro.Normalize()));
        mvprintw(s++, 0, "X: %.2f Y: %.2f Z: %.2f", SceneInstance.CameraInstance.Position.X,
                 SceneInstance.CameraInstance.Position.Y, SceneInstance.CameraInstance.Position.Z);
        mvprintw(s++, 0, "cols = %d, rows = %d, c/r = %.2f", cols, rows, (float)cols / rows);
        mvprintw(s++, 0, "looop time = %ld fixed FPS = %.2f", MS_PER_UPDATE.count(),
                 1000.0 / MS_PER_UPDATE.count());
    }
    lock.unlock();
    refresh();  // Aggiorna lo schermo
}

point2 Context::Project(const point3& obj, const Camera& cam) {
    point3 diff = obj - cam.Position;

    point3 up = cam.Direction.UnitVector().RotatePhi90Up();
    point3 right = cam.Direction.UnitVector().RotateTheta90YX();
    point3 pro = up.OddPart(diff);
    point3 bro = right.OddPart(diff);
    // return { wrapToPi(diff.Theta() - CameraInstance.Direction.Theta)*cos(diff.Phi()),
    // ((diff.Phi() - CameraInstance.Direction.Phi)) };

    return {atan2(up * cam.Direction.UnitVector().CrossProduct(pro.Normalize()),
                  cam.Direction.UnitVector() * pro.Normalize()),
            atan2(right * bro.Normalize().CrossProduct(cam.Direction.UnitVector()),
                  cam.Direction.UnitVector() * bro.Normalize())};
}

void Context::Update() {
    std::unique_lock<std::mutex> pock(InputStateMutex);
    InputState = InputStateRef;
    pock.unlock();

    auto scn = SceneInstance.Old();
    auto n_scn = SceneInstance.New();
    n_scn.CameraInstance = scn.CameraInstance;
    if (InputState[103]) {
        if (scn.CameraInstance.Direction.Phi <= M_PI_2) n_scn.CameraInstance.Direction.Phi += 0.1;
    }
    if (InputState[108]) {
        if (scn.CameraInstance.Direction.Phi >= -M_PI_2) n_scn.CameraInstance.Direction.Phi -= 0.1;
    }
    if (InputState[105]) {
        n_scn.CameraInstance.Direction.Theta -= 0.15;
    }
    if (InputState[106]) {
        n_scn.CameraInstance.Direction.Theta += 0.15;
    }
    n_scn.CameraInstance.Speed = point3();
    if (InputState[30]) {  // A
        n_scn.CameraInstance.Speed += point3(sin(scn.CameraInstance.Direction.Theta),
                                             -cos(scn.CameraInstance.Direction.Theta), 0.0);
    }
    if (InputState[32]) {  // D
        n_scn.CameraInstance.Speed += point3(-sin(scn.CameraInstance.Direction.Theta),
                                             cos(scn.CameraInstance.Direction.Theta), 0.0);
    }
    if (InputState[17]) {  // W
        n_scn.CameraInstance.Speed += point3(cos(scn.CameraInstance.Direction.Theta),
                                             sin(scn.CameraInstance.Direction.Theta), 0.0);
    }
    if (InputState[31]) {  // S
        n_scn.CameraInstance.Speed += point3(-cos(scn.CameraInstance.Direction.Theta),
                                             -sin(scn.CameraInstance.Direction.Theta), 0.0);
    }
    if (InputState[30] || InputState[32] || InputState[31] || InputState[17]) {
        n_scn.CameraInstance.Speed.Normalize();
        n_scn.CameraInstance.Speed *= scn.step;
    }

    n_scn.CameraInstance.Position += n_scn.CameraInstance.Speed;

    if (InputState[57]) {
        if (!scn.jumping) {
            n_scn.vz = 0.6;
            n_scn.g = -0.03;
            n_scn.jumping = true;
        }
    }

    n_scn.vz += n_scn.g;
    n_scn.CameraInstance.Position.Z += n_scn.vz;

    if (scn.CameraInstance.Position.Z < 7) {
        n_scn.CameraInstance.Position.Z = 7;
        n_scn.vz = 0;
        n_scn.g = 0;
        n_scn.jumping = false;
    }
    if (InputState[52]) {
    }
    if (InputState[53]) {
    }
    if (InputState[23]) {
        debug = !debug;
    }
}

void Context::Rasterizer() {
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
    for (int i = 0; i < scn.Points.size(); i++) {
        double distance = (scn.Points[i] - scn.CameraInstance.Position).Length();
        point2 projection = Project(scn.Points[i], scn.CameraInstance);
        int yy = (int)round(((projection.X / PixelRatio) + 0.5) * cols);
        int xx = (int)round(((-projection.Y) + 0.5) * rows);
        if ((xx >= 0) && (yy >= 0) && (xx < rows) && (yy < cols)) {
            if ((distance > 0) && ((Pixels[xx][yy].Distance != Pixels[xx][yy].Distance) ||
                                   (Pixels[xx][yy].Distance > distance))) {
                Pixels[xx][yy].Distance = distance;
                Pixels[xx][yy].Char = '+';
                Pixels[xx][yy].Colour = 7;
            }
        }
    }
    for (int i = 0; i < Pixels.size(); i++) {
        for (int j = 0; j < Pixels[0].size(); j++) {
            // point3 d = ( CameraInstance.Direction + direction3( (( (double)j / cols) -
            // 0.5)*2.06,  - (( (double)i / rows) - 0.5  ) )).UnitVector();
            point3 u = scn.CameraInstance.Direction.UnitVector().RotatePhi90Up();
            point3 r = scn.CameraInstance.Direction.UnitVector().RotateTheta90YX();
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

            for (int f = 0; f < scn.Triangles.size(); f++) {
                point3 B = scn.Triangles[f].b - scn.Triangles[f].a;
                point3 C = scn.Triangles[f].c - scn.Triangles[f].a;
                const point3& P0 = scn.Triangles[f].a;
                double det = B.Z * C.Y * d.X - B.Y * C.Z * d.X - B.Z * C.X * d.Y + B.X * C.Z * d.Y +
                             B.Y * C.X * d.Z - B.X * C.Y * d.Z;
                point3 k = scn.CameraInstance.Position - P0;
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
                        int a = rand() % 7;
                        char b = ' ';
                        switch (a) {
                            case 0:
                                b = 'R';
                                break;
                            case 1:
                                b = 'e';
                                break;
                            case 2:
                                b = 'v';
                                break;
                            case 3:
                                b = 'r';
                                break;
                            case 4:
                                b = 'b';
                                break;
                            case 5:
                                b = 'o';
                                break;
                            case 6:
                                b = 'i';
                        }
                        Pixels[i][j].Char = b;
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

void UpdateLoop(Context& ref, std::atomic<bool>& running) {
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        ref.GetDimensions();
        if ((ref.rows != ref.SceneInstance.Pixels.size()) ||
            (ref.cols != ref.SceneInstance.Pixels[0].size())) {
            std::lock_guard<std::mutex> lock(ref.bufferMutex);
            ref.SceneInstance.Pixels =
                std::vector<std::vector<Pixel>>(ref.rows, std::vector<Pixel>(ref.cols));
            ref.SceneInstance.Rixels =
                std::vector<std::vector<Pixel>>(ref.rows, std::vector<Pixel>(ref.cols));
        }

        ref.SceneInstance.Update();
        frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (frameDuration < MS_PER_UPDATE) {
            std::this_thread::sleep_for(MS_PER_UPDATE - frameDuration);
        }
    }
}