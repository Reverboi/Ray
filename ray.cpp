#include <ncurses.h>
#include <array>
#include <vector>
#include <cmath>
//using std::cos, std::sin, std::atan2, std::sqrt, std::abs, std::round;
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
	direction3 Direction;

    Camera( point3 p, direction3 d ) : Position( p ), Direction( d ) {};
};

struct Pixel
{
    char Char = ' ';
    int Colour = 0;
    double Distance = 0.0/0;
    Pixel ( char a ) : Char(a) {};
    Pixel () = default;
};
double wrapToPi(double angle) {
    angle = std::fmod(angle + M_PI, 2 * M_PI);
    if (angle < 0) angle += 2 * M_PI;
    return angle - M_PI;
}

class Context
{
    public:
    Camera Cam;
    double q = -0.3;
    double p = 0.0;
    double PixelRatio = 2.06; //2.06
    int NumberOfThings;
    std::vector<triangle3> Things;
    std::vector<point3> ThingsP;
    std::vector<point2> ImagesP;

    std::vector<std::vector<struct Pixel>> Pixels;
    Context( int x, int y ) :
        Pixels( y, std::vector<Pixel>( x ) ),
        Cam( {0, 0, 7}, {0, 0} ),
        Things({{
            {{ 10, 0, 10 }, { 10, 5, 5 }, { 10, -5, 5 }},
            {{ 8, -2, 0 }, { 4, -2, 0 }, { 6, 0, 6 }},
            {{ 8, +2, 0 }, { 4, +2, 0 }, { 6, 0, 6 }},
            {{ 8, -2, 0 }, { 8, +2, 0 }, { 6, 0, 6 }},
            {{ 4, -2, 0 }, { 4, +2, 0 }, { 6, 0, 6 }}
            }}),
        NumberOfThings(5)
    {
        int p=0;
        ThingsP.push_back({0,0,0});
        for (int _i=1; _i<=20; _i++)
        {
            double i = (double)_i;
            ThingsP.push_back({0,0,i});
            ThingsP.push_back({0,i,0});
            ThingsP.push_back({i,0,0});

            ThingsP.push_back({0,0,-i});
            ThingsP.push_back({0,-i,0});
            ThingsP.push_back({-i,0,0});
        }
        for(int _j=1; _j<=20;_j++){
            for (int _i=1; _i<=20; _i++)
            {
                double i = (double)_i, j = (double)_j;
                ThingsP.push_back({i,j,0});    //  floor
                ThingsP.push_back({-i,j,0});
                ThingsP.push_back({i,-j,0});
                ThingsP.push_back({-i,-j,0});

                ThingsP.push_back({0,i,j});  // wall
            }
            ThingsP.push_back({10, 5, 5});
            ThingsP.push_back({10, -5, 5});
            ThingsP.push_back({10, 0, 10});
        }
    }
    void ProjectAll()
    {
        ImagesP.clear();
        for( int i=0; i<ThingsP.size(); i++ )
        {   
            ImagesP.push_back(Project(ThingsP[i]));
        }
    }
    triangle2 Project(const triangle3& obj) { return { Project(obj.a), Project(obj.b), Project(obj.c) }; }
    point2 Project(const point3& obj)
    {
        point3 diff = obj - Cam.Position;
        
        point3 up = Cam.Direction.UnitVector().RotatePhi90Up();
        point3 right = Cam.Direction.UnitVector().RotateTheta90YX();
        point3 pro = up.OddPart(diff);
        point3 bro = right.OddPart(diff);
        //return { wrapToPi(diff.Theta() - Cam.Direction.Theta)*cos(diff.Phi()), ((diff.Phi() - Cam.Direction.Phi)) };

        return {
            atan2(up * Cam.Direction.UnitVector().CrossProduct(pro.Normalize()), Cam.Direction.UnitVector()*pro.Normalize()),
            atan2(right * bro.Normalize().CrossProduct(Cam.Direction.UnitVector()), Cam.Direction.UnitVector()*bro.Normalize() )
        };

    }
    void Cast()
    {
        //dato un pixel a x-y
        //se ne calcola il vettore unitario corrispondente
        int rows = Pixels.size();
        int cols = Pixels[0].size();
        for (int i = 0; i<Pixels.size(); i++){
            for (int j = 0; j<Pixels[0].size(); j++){
            //point3 d = ( Cam.Direction + direction3( (( (double)j / cols) - 0.5)*2.06,  - (( (double)i / rows) - 0.5  ) )).UnitVector();
            point3 u = Cam.Direction.UnitVector().RotatePhi90Up();
            point3 r = Cam.Direction.UnitVector().RotateTheta90YX();
            point3 l = u.Rotate(r, ((( (double)j / cols) - 0.5)*PixelRatio));
            point3 h = r.Rotate(u,(( (double)i / rows) - 0.5));
            //d = u.CrossProduct(d).Rotate(d, (( (double)i / rows) - 0.5)*cos((( (double)j / cols) - 0.5)*PixelRatio)*(1-cos(( (double)i / rows) - 0.5)));
            point3 d = l.CrossProduct(h).Normalize();
            /*point3 d = Cam.Direction.UnitVector().RotatePhi90Up().Rotate(
                Cam.Direction.UnitVector().RotateTheta90YX().Rotate(
                        Cam.Direction.UnitVector(), 
                        tan(( (double)i / rows) - 0.5) ), ((( (double)j / cols) - 0.5)*2.06));*/
            //point3 d = ( Cam.Direction + direction3( (( (double)j / cols) - 0.5) *2.06, - (( (double)i / rows) - 0.5 ) ) ).UnitVector();
            //si risolve per tutti i piani/triangoli
            double min_t = 0.0/0;
            double min_t_u = 0, min_t_v = 0;
            Pixels[i][j].Char = ' ';
            Pixels[i][j].Distance = 0.0/0;
            for(int f=0; f<NumberOfThings; f++)
                {
                    point3 B = Things[f].b - Things[f].a;
                    point3 C = Things[f].c - Things[f].a;
                    const point3& P0 = Things[f].a;
                    double det = B.Z * C.Y * d.X - B.Y * C.Z * d.X - B.Z * C.X * d.Y + B.X * C.Z * d.Y + B.Y * C.X * d.Z - B.X * C.Y * d.Z;
                    point3 k = Cam.Position - P0;
                    double t = point3(-B.Z * C.Y + B.Y * C.Z, B.Z * C.X - B.X * C.Z, -B.Y * C.X + B.X * C.Y) * k / det;
                    double u = point3(C.Z * d.Y - C.Y * d.Z, -C.Z * d.X + C.X * d.Z, C.Y * d.X - C.X * d.Y) * k / det;
                    double v = point3(-B.Z * d.Y + B.Y * d.Z, B.Z * d.X - B.X * d.Z, -B.Y * d.X + B.X * d.Y) * k / det;
                    //si salva il carattere corrispondente alla distanza minore
                    if ((t>0)&&(u>=0)&&(v>=0)&&(u<=1)&&(v<=1)&&(u+v<=1)) {
                        if ((t<min_t)||(min_t!=min_t)) {
                            min_t = t; min_t_u = u; min_t_v = v; 
                            Pixels[i][j].Char = '#';
                            Pixels[i][j].Colour = f+1;
                            Pixels[i][j].Distance = t;
                        }
                    }
                }
            }
        }
        for(int i= 0; i<ImagesP.size(); i++){
            double distance = (ThingsP[i] - Cam.Position).Length();
            int yy = (int)round(((ImagesP[i].X / PixelRatio )+0.5)*cols);
            int xx = (int)round(((-ImagesP[i].Y)+0.5)*rows);
            if((xx>=0)&&(yy>=0)&&(xx<rows)&&(yy<cols)){
            if ((distance>0)&&((Pixels[xx][yy].Distance>distance)||(Pixels[xx][yy].Distance!=Pixels[xx][yy].Distance))){
                Pixels[xx][yy].Distance = distance;
                Pixels[xx][yy].Char = '+';
                Pixels[xx][yy].Colour = 7;
            }}
        }
    }
};