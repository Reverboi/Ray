#include <ncurses.h>
#include <array>
#include <vector>
#include <cmath>
#include <chrono>
#include <mutex>
struct direction3; // Forward declaration for point3.

struct point3 
{
    double X;
    double Y;
    double Z;

    point3(double a = 0, double b = 0, double c = 0) : X(a), Y(b), Z(c) {}

    point3 operator*(const double l) const { return {l * X, l * Y, l * Z}; }
    point3 operator/(const double l) const { return {X / l, Y / l, Z / l}; }

    point3(const direction3& d, const double len = 1);

    double operator*(const point3& other) const { return X * other.X + Y * other.Y + Z * other.Z; }

    double Length() const { return std::sqrt(*this * *this); }

    double Theta() const { return std::atan2( Y, X ); } // Angle on the X-Y plane.
    double Phi() const { return std::atan2( Z, std::sqrt( X * X + Y * Y ) ); } // Angle from the X-Y plane.

    direction3 Direction() const;

    point3(const point3&) = default;
    point3(point3&&) = default;
    point3& operator=(const point3&) = default;
    point3& operator=(point3&&) = default;
    point3 Normalize() const {return *this/Length();}
    point3 operator+(const point3& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
    point3 operator-(const point3& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }
    point3 EvenPart(const point3& other) const { return (*this)*(*this * other)/(*this * *this);}
    point3 OddPart(const point3& other) const { return other - EvenPart(other);}
    point3 CrossProduct(const point3& other) const { return { Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X }; }
    double AngleFrom(const point3& other) const {  // bad
        point3 a = Normalize();
        point3 b = other.Normalize();
        return atan2(a.CrossProduct(b).Length(),a*b);
    }
    point3 Rotate(const point3& o, double t) const {
        point3 d = Normalize();
        return {
            (cos(t)+d.X*d.X*(1-cos(t)))*o.X + (d.X*d.Y*(1-cos(t))-d.Z*sin(t))*o.Y + (d.X*d.Z*(1-cos(t))+d.Y*sin(t))*o.Z,
            (d.Y*d.X*(1-cos(t))+d.Z*sin(t))*o.X + (cos(t)+d.Y*d.Y*(1-cos(t)))*o.Y + (d.Y*d.Z*(1-cos(t))-d.X*sin(t))*o.Z,
            (d.Z*d.X*(1-cos(t))-d.Y*sin(t))*o.X + (d.Z*d.Y*(1-cos(t))+d.X*sin(t))*o.Y + (cos(t)+d.Z*d.Z*(1-cos(t)))*o.Z
        };
    }
    point3 RotatePhi90Up()
    {
        return
        {
            - Z * X / sqrt( X * X + Y * Y ),
            - Z * Y / sqrt( X * X + Y * Y ),
            sqrt( X * X + Y * Y )
        };
    }
    point3 RotateTheta90YX() { return RotatePhi90Up().CrossProduct(*this);} // does not preserve module if!=1
};

struct direction3 
{
    double Theta;
    double Phi;

    direction3(const point3& p);
    direction3(double a, double b) : Theta(a), Phi(b) {}

    double X() const { return std::cos(Theta) * std::cos(Phi); }
    double Y() const { return std::sin(Theta) * std::cos(Phi); }
    double Z() const { return std::sin(Phi); }

    point3 UnitVector() const { return { X(), Y(), Z() }; }
    direction3(const direction3&) = default;
    direction3(direction3&&) = default;
    direction3() = default;
    direction3& operator=(const direction3&) = default;
    direction3& operator=(direction3&&) = default;

    direction3 operator+(const direction3& o) { return { Theta+o.Theta, Phi+o.Phi }; };
    direction3 operator-(const direction3& o) { return { Theta-o.Theta, Phi-o.Phi }; };
};

point3::point3(const direction3& d, const double len) : point3(d.UnitVector() * len) {}

direction3 point3::Direction() const { return { Theta(), Phi() }; }
direction3::direction3(const point3& a) : direction3(a.Direction()) {}

struct point2
{
	double X;
	double Y;
	point2( double a, double b ) : X( a ), Y( b ) {};
	point2()
    {
		X = 0.0;
		Y = 0.0;
	}
};

struct triangle3
{
	point3 a,b,c;
    int Colour = 1;
	triangle3( const point3& a, const point3& b, const point3& c ) : a( a ), b( b ), c( c ) {};
    triangle3() = default;
};

struct triangle2
{
	point2 a,b,c;
	triangle2( const point2& a, const point2& b, const point2& c ) : a( a ), b( b ), c( c ) {};
    triangle2() = default;
    double Area() const { return abs( ( a.X * ( b.Y - c.Y ) + b.X * ( c.Y - a.Y ) + c.X * ( a.Y - b.Y ) ) / 2.0 ); }
};

class Camera
{
    public:
	point3 Position;
    point3 Velocity;
	direction3 Direction;
    direction3 DirectionDelta;

    Camera( point3 p, direction3 d ) : Position( p ), Direction( d ), Velocity(), DirectionDelta() {};
};

struct Pixel
{
    char Char = ' ';
    int Colour = 0;
    double Distance = NAN;
    Pixel ( char a ) : Char(a) {};
    Pixel () = default;
};
double wrapToPi(double angle) {
    angle = std::fmod(angle + M_PI, 2 * M_PI);
    if (angle < 0) angle += 2 * M_PI;
    return angle - M_PI;
}

constexpr int FPS = 28; // do these two really need to be consdtexpr? what if i want  to be able to change fps mid-game for any reasson
constexpr std::chrono::milliseconds MS_PER_FRAME(1000 / FPS); // if you keep them contexpr and bring them inside Scene you'll need static as well

class Scene
{
    public:
    Camera CameraInstance;
    double PixelRatio = 2.06; //2.06
    bool jumping = false;
    bool debug = true;
    double step = 0.6;
    double g=0, vz=0;
    int frameDuration; // milliseconds
    std::vector<triangle3> Triangles;
    std::vector<point3> Points;

    std::array<bool, 256>& InputStateRef; // it'd be nice to wrap them toghether?
    std::array<bool, 256> InputState;
    std::mutex& InputStateMutex;

    std::vector<std::vector<struct Pixel>> Pixels;
    Scene(std::array<bool, 256>& key_states, std::mutex& key_mutex, int x, int y ) :
        Pixels( y, std::vector<Pixel>( x ) ),
        InputStateRef(key_states),
        InputStateMutex(key_mutex),
        CameraInstance( {0, 0, 7}, {0, 0} ),
        Triangles({{
            {{ 10, 0, 10 }, { 10, 5, 5 }, { 10, -5, 5 }},
            {{ 8, -2, 0 }, { 4, -2, 0 }, { 6, 0, 6 }},
            {{ 8, +2, 0 }, { 4, +2, 0 }, { 6, 0, 6 }},
            {{ 8, -2, 0 }, { 8, +2, 0 }, { 6, 0, 6 }},
            {{ 4, -2, 0 }, { 4, +2, 0 }, { 6, 0, 6 }}
            }})
        {
        Points.push_back({0,0,0});
        for (int _i=1; _i<=20; _i++)
        {
            double i = (double)_i;
            Points.push_back({0,0,i});
            Points.push_back({0,i,0});
            Points.push_back({i,0,0});

            Points.push_back({0,0,-i});
            Points.push_back({0,-i,0});
            Points.push_back({-i,0,0});
        }
        for(int _j=1; _j<=20;_j++){
            for (int _i=1; _i<=20; _i++)
            {
                double i = (double)_i, j = (double)_j;
                Points.push_back({i,j,0});    //  floor
                Points.push_back({-i,j,0});
                Points.push_back({i,-j,0});
                Points.push_back({-i,-j,0});

                Points.push_back({0,i,j});  // wall
            }
            Points.push_back({10, 5, 5});
            Points.push_back({10, -5, 5});
            Points.push_back({10, 0, 10});
        }
    }
    point2 Project(const point3& obj)
    {
        point3 diff = obj - CameraInstance.Position;
        
        point3 up = CameraInstance.Direction.UnitVector().RotatePhi90Up();
        point3 right = CameraInstance.Direction.UnitVector().RotateTheta90YX();
        point3 pro = up.OddPart(diff);
        point3 bro = right.OddPart(diff);
        //return { wrapToPi(diff.Theta() - CameraInstance.Direction.Theta)*cos(diff.Phi()), ((diff.Phi() - CameraInstance.Direction.Phi)) };

        return {
            atan2(up * CameraInstance.Direction.UnitVector().CrossProduct(pro.Normalize()), CameraInstance.Direction.UnitVector()*pro.Normalize()),
            atan2(right * bro.Normalize().CrossProduct(CameraInstance.Direction.UnitVector()), CameraInstance.Direction.UnitVector()*bro.Normalize() )
        };

    }
    void Update()
    {
        std::unique_lock<std::mutex> pock(InputStateMutex);
        InputState = InputStateRef;
        pock.unlock();

        if (InputState[103]){
            if ( CameraInstance.Direction.Phi <= M_PI_2 )
                CameraInstance.Direction.Phi += 0.1;
        }
        if (InputState[108]){
            if ( CameraInstance.Direction.Phi >= - M_PI_2 )
            CameraInstance.Direction.Phi -= 0.1;
        }
        if (InputState[105]){
            CameraInstance.Direction.Theta -= 0.15;
        }
        if (InputState[106]){
            CameraInstance.Direction.Theta += 0.15;
        }
        if (InputState[30]){
            CameraInstance.Position.Y -= step * cos(CameraInstance.Direction.Theta);
            CameraInstance.Position.X += step * sin(CameraInstance.Direction.Theta);
        }
        if (InputState[32]){
            CameraInstance.Position.Y += step * cos(CameraInstance.Direction.Theta);
            CameraInstance.Position.X -= step * sin(CameraInstance.Direction.Theta);
        }
        if (InputState[17]){
            CameraInstance.Position.X += step * cos(CameraInstance.Direction.Theta);
            CameraInstance.Position.Y += step * sin(CameraInstance.Direction.Theta);
        }
        if (InputState[31]){
            CameraInstance.Position.X -= step * cos(CameraInstance.Direction.Theta);
            CameraInstance.Position.Y -= step * sin(CameraInstance.Direction.Theta);
        }
        if (InputState[57]){
            if (!jumping){
                vz = 0.6;
                g = -0.03;
                jumping = true;
            }
        }
        CameraInstance.Position.Z += vz;
        vz +=g;
        if(CameraInstance.Position.Z<7){
            CameraInstance.Position.Z = 7;
            vz = 0;
            g = 0;
            jumping = false;
        }
        if (InputState[52]){
        }
        if (InputState[53]){
        }
        if (InputState[23]){
            debug = !debug;
        }  
        //dato un pixel a x-y
        //se ne calcola il vettore unitario corrispondente
        int rows = Pixels.size();
        int cols = Pixels[0].size();
        for (int i = 0; i<Pixels.size(); i++){
            for (int j = 0; j<Pixels[0].size(); j++){
            //point3 d = ( CameraInstance.Direction + direction3( (( (double)j / cols) - 0.5)*2.06,  - (( (double)i / rows) - 0.5  ) )).UnitVector();
            point3 u = CameraInstance.Direction.UnitVector().RotatePhi90Up();
            point3 r = CameraInstance.Direction.UnitVector().RotateTheta90YX();
            point3 l = u.Rotate(r, ((( (double)j / cols) - 0.5)*PixelRatio));
            point3 h = r.Rotate(u,(( (double)i / rows) - 0.5));
            //d = u.CrossProduct(d).Rotate(d, (( (double)i / rows) - 0.5)*cos((( (double)j / cols) - 0.5)*PixelRatio)*(1-cos(( (double)i / rows) - 0.5)));
            point3 d = l.CrossProduct(h).Normalize();
            /*point3 d = CameraInstance.Direction.UnitVector().RotatePhi90Up().Rotate(
                CameraInstance.Direction.UnitVector().RotateTheta90YX().Rotate(
                        CameraInstance.Direction.UnitVector(), 
                        tan(( (double)i / rows) - 0.5) ), ((( (double)j / cols) - 0.5)*2.06));*/
            //point3 d = ( CameraInstance.Direction + direction3( (( (double)j / cols) - 0.5) *2.06, - (( (double)i / rows) - 0.5 ) ) ).UnitVector();
            //si risolve per tutti i piani/triangoli
            double min_t = NAN;
            Pixels[i][j].Char = ' ';
            Pixels[i][j].Distance = NAN;
            for(int f=0; f<Triangles.size(); f++)
                {
                    point3 B = Triangles[f].b - Triangles[f].a;
                    point3 C = Triangles[f].c - Triangles[f].a;
                    const point3& P0 = Triangles[f].a;
                    double det = B.Z * C.Y * d.X - B.Y * C.Z * d.X - B.Z * C.X * d.Y + B.X * C.Z * d.Y + B.Y * C.X * d.Z - B.X * C.Y * d.Z;
                    point3 k = CameraInstance.Position - P0;
                    double t = point3(-B.Z * C.Y + B.Y * C.Z, B.Z * C.X - B.X * C.Z, -B.Y * C.X + B.X * C.Y) * k / det;
                    double u = point3(C.Z * d.Y - C.Y * d.Z, -C.Z * d.X + C.X * d.Z, C.Y * d.X - C.X * d.Y) * k / det;
                    double v = point3(-B.Z * d.Y + B.Y * d.Z, B.Z * d.X - B.X * d.Z, -B.Y * d.X + B.X * d.Y) * k / det;
                    //si salva il carattere corrispondente alla distanza minore
                    if ((t>0)&&(u>=0)&&(v>=0)&&(u<=1)&&(v<=1)&&(u+v<=1)) {
                        if ((t<min_t)||(min_t!=min_t)) {
                            min_t = t; 
                            Pixels[i][j].Char = '#';
                            Pixels[i][j].Colour = f+1;
                            Pixels[i][j].Distance = t;
                        }
                    }
                }
            }
        }
        for(int i= 0; i<Points.size(); i++){
            double distance = (Points[i] - CameraInstance.Position).Length();
            point2 projection = Project(Points[i]);
            int yy = (int)round(((projection.X / PixelRatio )+0.5)*cols);
            int xx = (int)round(((-projection.Y)+0.5)*rows);
            if((xx>=0)&&(yy>=0)&&(xx<rows)&&(yy<cols)){
            if ((distance>0)&&((Pixels[xx][yy].Distance>distance)||(Pixels[xx][yy].Distance!=Pixels[xx][yy].Distance))){
                Pixels[xx][yy].Distance = distance;
                Pixels[xx][yy].Char = '+';
                Pixels[xx][yy].Colour = 7;
            }}
        }
    }
    void Render(){
        clear();
        Update();
        const int rows = Pixels.size();
        const int cols = Pixels[0].size();
        for (int y = 0; y < rows; ++y){
            for (int x = 0; x < cols; ++x){   
                attron(COLOR_PAIR(Pixels[y][x].Colour));
                mvaddch( y, x, Pixels[y][x].Char ) ;
                attroff(COLOR_PAIR(Pixels[y][x].Colour));
            }
        }
            
        int s=0;
        if(debug){
            point3 diff = point3(10,0,10) - CameraInstance.Position;
            point3 up = CameraInstance.Direction.UnitVector().RotatePhi90Up();
            point3 right = CameraInstance.Direction.UnitVector().RotateTheta90YX();
            point3 pro = up.OddPart(diff);
            point3 bro = right.OddPart(diff);
            mvprintw(s++, 0, "theta---: %.2f phi---: %.2f", CameraInstance.Direction.Theta, CameraInstance.Direction.Phi);
            mvprintw(s++, 0, "thetapro: %.2f phipro: %.2f", pro.Theta(), pro.Phi());
            mvprintw(s++, 0, "thetabro: %.2f phibro: %.2f", bro.Theta(), bro.Phi());
            mvprintw(s++, 0, "diff-len: %.2f diff-norm-len: %.2f", diff.Length(), diff.Normalize().Length());
            mvprintw(s++, 0, "thetadif: %.2f phidif: %.2f", atan2(up * CameraInstance.Direction.UnitVector().CrossProduct(pro.Normalize()), CameraInstance.Direction.UnitVector()*pro.Normalize()),
            atan2(right * CameraInstance.Direction.UnitVector().CrossProduct(bro.Normalize()), CameraInstance.Direction.UnitVector()*bro.Normalize() ));
            mvprintw(s++, 0, "X: %.2f Y: %.2f Z: %.2f", CameraInstance.Position.X, CameraInstance.Position.Y, CameraInstance.Position.Z);
            mvprintw(s++, 0, "cols = %d, rows = %d, c/r = %.2f", cols, rows, (float)cols/rows);
            mvprintw(s++, 0, "frame time = %d teomaxFPS = %.2f", frameDuration, 1000.0/frameDuration);
            mvprintw(s++, 0, "looop time = %ld fixed FPS = %.2f", MS_PER_FRAME.count(), 1000.0/MS_PER_FRAME.count());
        }
        refresh();  // Aggiorna lo schermo
    }
};