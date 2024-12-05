#include <curses.h> // Libreria curses
#include <array>
#include <cmath>
#define MAX_NUMBER_OF_THINGS 4
using std::cos, std::sin, std::atan2, std::sqrt, std::abs, std::round;
struct direction3; // Forward declaration for point3.

struct point3 
{
    double X;
    double Y;
    double Z;

    point3(double a = 0, double b = 0, double c = 0) : X(a), Y(b), Z(c) {}

    point3 operator*(const double l) const { return {l * X, l * Y, l * Z}; }
    point3 operator/(const double l) const { return {l / X, l / Y, l / Z}; }

    point3(const direction3& d, const double len = 1);

    double operator*(const point3& other) const { return X * other.X + Y * other.Y + Z * other.Z; }

    double Length() const { return std::sqrt(*this * *this); }

    double Theta() const { return std::atan2(Y, X); } // Angle on the X-Y plane.
    double Phi() const { return std::atan2(Z, std::sqrt(X * X + Y * Y)); } // Angle from Z-axis.

    direction3 Direction() const;

    point3(const point3&) = default;
    point3(point3&&) = default;
    point3& operator=(const point3&) = default;
    point3& operator=(point3&&) = default;

    point3 operator+(const point3& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
    point3 operator-(const point3& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }

    point3 RotatePhi90Up()
    {
        return
        {
            - Z * X / sqrt( X * X + Y * Y ),
            - Z * Y / sqrt( X * X + Y * Y ),
            sqrt( X * X + Y * Y )
        };
    }
    point3 RotateTheta90YX() { return { Y, -X, Z}; }
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
    direction3 Saggitario() const { return { Theta - M_PI_2, 0 }; }
    direction3(const direction3&) = default;
    direction3(direction3&&) = default;
    direction3& operator=(const direction3&) = default;
    direction3& operator=(direction3&&) = default;
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
    //std::array< point3, 3 > Vertices;
	point3 a,b,c;
	triangle3( const point3& a, const point3& b, const point3& c ) : a( a ), b( b ), c( c )/*, Vertices({a,b,c})*/ {};
    triangle3() = default;
};

struct triangle2
{
	point2 a,b,c;
	triangle2( const point2& a, const point2& b, const point2& c ) : a( a ), b( b ), c( c ) {};
    triangle2() = default;
    double Area() const { return abs( ( a.X * ( b.Y - c.Y ) + b.X * ( c.Y - a.Y ) + c.X * ( a.Y - b.Y ) ) / 2.0 ); }
    bool IsInside(const point2& p) const
    {
        double A1 = triangle2(p,b,c).Area();
        double A2 = triangle2(a,p,c).Area();
        double A3 = triangle2(a,b,p).Area();
        return abs(Area() - (A1 + A2 + A3)) <= 0.000001;
    }
};

class Camera
{
    public:
	point3 Position;
	direction3 Direction;
    Camera( point3 p, direction3 d ) : Position( p ), Direction( d ) {};
};

class Context
{
    public:
    Camera Cam;
    int NumberOfThings;
    std::array<triangle3, MAX_NUMBER_OF_THINGS> Things;
    std::array<triangle2, MAX_NUMBER_OF_THINGS> Images;
    std::array<point3, 2121> ThingsP;
    std::array<point2, 2121> ImagesP;
    Context() : Cam( {0, 0, 7}, {0, 0}), Things({{{{ 10, 5, 5 }, { 10, -5, 5 }, { 10, 0, 10 }}}}), NumberOfThings(1)
    {
        int p=0;
        ThingsP[p++] = {0,0,0};
        for (int i=1; i<=20; i++)
        {
            ThingsP[p++]={0,0,i};
            ThingsP[p++]={0,i,0};
            ThingsP[p++]={i,0,0};

            ThingsP[p++]={0,0,-i};
            ThingsP[p++]={0,-i,0};
            ThingsP[p++]={-i,0,0};
        }
        for(int j=1; j<=20;j++){
            for (int i=1; i<=20; i++)
            {
                ThingsP[p++]={i,j,0};
                ThingsP[p++]={-i,j,0};
                ThingsP[p++]={i,-j,0};
                ThingsP[p++]={-i,-j,0};

                ThingsP[p++]={0,i,j};
            }
        }
    }
    void ProjectAll()
    {
        for( int i=0; i<NumberOfThings; i++ )
        {
            Images[i] = Project(Things[i]);
        }
        for( int i=0; i<2121; i++ )
        {
            ImagesP[i] = Project(ThingsP[i]);
        }
    }
    triangle2 Project(const triangle3& obj) { return { Project(obj.a), Project(obj.b), Project(obj.c) }; }
    point2 Project(const point3& obj)
    {
        point3 diff = obj - Cam.Position;
        return { 2*tan((diff.Theta() - Cam.Direction.Theta)/2), 2*tan((diff.Phi() - Cam.Direction.Phi)/2) };
    }
    char Hits(const point2& p) const
    {
        for( int i=0; i<NumberOfThings; i++ )
        {
            if (Images[i].IsInside(p)) return '#';
        }
        return ' ';
    }
};

int main() {
    // Inizializzazione di curses
    initscr();             // Inizializza il terminale in modalità curses
    cbreak();              // Disabilita il buffering dell'input
    noecho();              // Non mostra i caratteri digitati dall'utente
    keypad(stdscr, TRUE);  // Abilita l'uso dei tasti freccia e funzioni speciali
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    
    Context Con;
    printf("\033[?1003h\n");
    getch();
    refresh(); // Aggiorna lo schermo per visualizzare il testo
	int rows, cols;
    getmaxyx(stdscr, rows, cols);
    double q=1;
    double qx=1;
    double step = 0.3;
    MEVENT event;
    int ch;
    
    while ((ch = getch()) != 'q') { // Finché l'utente non preme 'q'
        switch (ch) {
            case KEY_UP:
                Con.Cam.Position.X += step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.Y += step * sin(Con.Cam.Direction.Theta);
                break;
            case KEY_DOWN:
                Con.Cam.Position.X -= step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.Y -= step * sin(Con.Cam.Direction.Theta);
                break;
            case KEY_RIGHT:
                Con.Cam.Position.Y += step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.X -= step * sin(Con.Cam.Direction.Theta);
                break;
            case KEY_LEFT:
                Con.Cam.Position.Y -= step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.X += step * sin(Con.Cam.Direction.Theta);
                break;
            case 'g':
                qx += 0.02;
                break;
            case 'h':
                qx -= 0.02;
                break;
            case KEY_MOUSE:
                if ((getmouse(&event) == OK)) {
                Con.Cam.Direction.Theta = 2 * M_PI * ( ( (double)event.x / cols ) - 0.5 );
                Con.Cam.Direction.Phi = - M_PI * ( ( (double)event.y / rows ) - 0.5 );
                }
                break;
            default:
                break;
        }
        clear();
        Con.ProjectAll();
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                mvaddch( y, x, Con.Hits({ qx*(((double)x / cols) - 0.5), - q * (((double)y / rows) - 0.5) } ) ) ; // Sposta il cursore e aggiungi il carattere
            }
        }
        for(int i= 0; i<2121; i++){
            mvaddch( (int)round(((-Con.ImagesP[i].Y / q)+0.5)*rows), (int)round(((Con.ImagesP[i].X / qx)+0.5)*cols), '+' );
        }
        mvprintw(0, 0, "theta: %.2f phi: %.2f", Con.Cam.Direction.Theta, Con.Cam.Direction.Phi);
        mvprintw(1, 0, "X: %.2f Y: %.2f Z: %.2f", Con.Cam.Position.X, Con.Cam.Position.Y, Con.Cam.Position.Z);
        mvprintw(2, 0, "a = (%.2f, %.2f)", Con.Images[0].a.X, Con.Images[0].a.Y);
        mvprintw(3, 0, "b = (%.2f, %.2f)", Con.Images[0].b.X, Con.Images[0].b.Y);
        mvprintw(4, 0, "a = (%.2f, %.2f, %.2f)", Con.Things[0].a.X, Con.Things[0].a.Y, Con.Things[0].a.Z);
        mvprintw(5, 0, "b = (%.2f, %.2f, %.2f)", Con.Things[0].b.X, Con.Things[0].b.Y, Con.Things[0].b.Z);
        mvprintw(6, 0, "qx = %.2f", qx);
        refresh();  // Aggiorna lo schermo
    }

    // Chiudi curses e ripristina il terminale
    endwin();
    printf("\033[?1003l\n");
    return 0;
}