#include <libevdev/libevdev.h>
#include <libudev.h>
#include <ncurses.h>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include "ray.cpp"

struct KeyInfo {
    std::string device_name;
    std::string state;
    bool is; //hack
};

std::map<int, KeyInfo> key_states;
std::mutex key_mutex;
std::atomic<bool> running(true);

struct DeviceContext {
    int fd;
    libevdev* dev;
    std::thread thread;
    std::string devnode;
    std::string name;
};

std::mutex device_mutex;
std::map<std::string, DeviceContext> device_threads;

void signalHandler(int signum) {
    running = false;
}

void readDeviceEvents(const std::string& devnode, libevdev* dev, const std::string& name) {
    struct input_event ev;
    while (running) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == 0 && ev.type == EV_KEY) {
            std::lock_guard<std::mutex> lock(key_mutex);
            if (ev.value == 1 || ev.value == 2) {
                key_states[ev.code] = {name, ev.value == 1 ? "Pressed" : "Held", true}; //hack
            } else if (ev.value == 0) {
                key_states.erase(ev.code);
            }
        } else if (rc < 0 && rc != -EAGAIN) {
            break;
        }
    }
}

void addDevice(struct udev_device* dev) {
    const char* devnode = udev_device_get_devnode(dev);
    if (!devnode) return;

    int fd = open(devnode, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return;

    libevdev* evdev = nullptr;
    if (libevdev_new_from_fd(fd, &evdev) < 0) {
        close(fd);
        return;
    }

    if (!libevdev_has_event_type(evdev, EV_KEY)) {
        libevdev_free(evdev);
        close(fd);
        return;
    }

    std::string devnode_str(devnode);
    std::string name = libevdev_get_name(evdev);

    DeviceContext ctx;
    ctx.fd = fd;
    ctx.dev = evdev;
    ctx.devnode = devnode_str;
    ctx.name = name;
    ctx.thread = std::thread(readDeviceEvents, devnode_str, evdev, name);

    std::lock_guard<std::mutex> lock(device_mutex);
    device_threads[devnode_str] = std::move(ctx);
}

void removeDevice(const std::string& devnode) {
    std::lock_guard<std::mutex> lock(device_mutex);
    auto it = device_threads.find(devnode);
    if (it != device_threads.end()) {
        close(it->second.fd);
        if (it->second.thread.joinable())
            it->second.thread.detach();
        libevdev_free(it->second.dev);
        device_threads.erase(it);
    }
}

void monitorDevices() {
    struct udev* udev = udev_new();
    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
    udev_monitor_enable_receiving(mon);
    int fd = udev_monitor_get_fd(mon);

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_property(enumerate, "ID_INPUT_KEYBOARD", "1");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);
        addDevice(dev);
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval tv = {0, 500000};
        if (select(fd+1, &fds, NULL, NULL, &tv) > 0) {
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                std::string action = udev_device_get_action(dev);
                std::string devnode = udev_device_get_devnode(dev) ? udev_device_get_devnode(dev) : "";

                if (action == "add") {
                    if (udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD"))
                        addDevice(dev);
                } else if (action == "remove") {
                    if (!devnode.empty())
                        removeDevice(devnode);
                }

                udev_device_unref(dev);
            }
        }
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
}

constexpr int FPS = 28;
constexpr std::chrono::milliseconds MS_PER_FRAME(1000 / FPS);

void displayLoop() {
    // Inizializzazione di curses
    initscr();             // Inizializza il terminale in modalità curses
    start_color(); // Enable color functionality

    // Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    cbreak();              // Disabilita il buffering dell'input
    noecho();
    // nodelay(stdscr, TRUE); makes getch() impatient
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    Context Con(cols, rows);
    refresh(); // Aggiorna lo schermo per visualizzare il testo
	bool jumping = false;
    bool debug = true;
    double qx=1.0;
    double step = 0.6;
    double g=0, vz=0;
    auto frameStart = std::chrono::high_resolution_clock::now();
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
    while (running) {
        frameStart = std::chrono::high_resolution_clock::now();

        getmaxyx(stdscr, rows, cols);
        if ((rows!=Con.Pixels.size())||(cols!=Con.Pixels[0].size())){
            Con.Pixels = std::vector<std::vector<Pixel>>( rows, std::vector<Pixel>( cols ) );
        }
        std::unique_lock<std::mutex> lock(key_mutex);
        //key_mutex.lock();
        if (!key_states.empty()) { 
            if (key_states[103].is){
                if ( Con.Cam.Direction.Phi <= M_PI_2 )
                    Con.Cam.Direction.Phi += 0.1;
            }
            if (key_states[108].is){
                if ( Con.Cam.Direction.Phi >= - M_PI_2 )
                Con.Cam.Direction.Phi -= 0.1;
            }
            if (key_states[105].is){
                Con.Cam.Direction.Theta -= 0.15;
            }
            if (key_states[106].is){
                Con.Cam.Direction.Theta += 0.15;
            }
            if (key_states[30].is){
                Con.Cam.Position.Y -= step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.X += step * sin(Con.Cam.Direction.Theta);
            }
            if (key_states[32].is){
                Con.Cam.Position.Y += step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.X -= step * sin(Con.Cam.Direction.Theta);
            }
            if (key_states[17].is){
                Con.Cam.Position.X += step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.Y += step * sin(Con.Cam.Direction.Theta);
            }
            if (key_states[31].is){
                Con.Cam.Position.X -= step * cos(Con.Cam.Direction.Theta);
                Con.Cam.Position.Y -= step * sin(Con.Cam.Direction.Theta);
            }
            if (key_states[57].is){
                if (!jumping){
                    vz = 0.6;
                    g = -0.03;
                    jumping = true;
                }
            }
            Con.Cam.Position.Z += vz;
            vz +=g;
            if(Con.Cam.Position.Z<7){
                Con.Cam.Position.Z = 7;
                vz = 0;
                g = 0;
                jumping = false;
            }
            if (key_states[52].is){
                Con.p-=0.02;
            }
            if (key_states[53].is){
                Con.p+=0.02;
            }
            if (key_states[23].is){
                debug = !debug;
            }
        }
        clear();
        Con.ProjectAll();
        Con.Cast();
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {   
                attron(COLOR_PAIR(Con.Pixels[y][x].Colour));
                mvaddch( y, x, Con.Pixels[y][x].Char ) ;
                attroff(COLOR_PAIR(Con.Pixels[y][x].Colour));
            }
        }
        
        int s=0;
        if(debug){
            point3 diff = point3(10,0,10) - Con.Cam.Position;
        
            point3 up = Con.Cam.Direction.UnitVector().RotatePhi90Up();
            point3 right = Con.Cam.Direction.UnitVector().RotateTheta90YX();
            point3 pro = up.OddPart(diff);
            point3 bro = right.OddPart(diff);
        mvprintw(s++, 0, "theta---: %.2f phi---: %.2f", Con.Cam.Direction.Theta, Con.Cam.Direction.Phi);
        mvprintw(s++, 0, "thetapro: %.2f phipro: %.2f", pro.Theta(), pro.Phi());
        mvprintw(s++, 0, "thetabro: %.2f phibro: %.2f", bro.Theta(), bro.Phi());
        mvprintw(s++, 0, "diff-len: %.2f diff-norm-len: %.2f", diff.Length(), diff.Normalize().Length());
        mvprintw(s++, 0, "thetadif: %.2f phidif: %.2f", atan2(up * Con.Cam.Direction.UnitVector().CrossProduct(pro.Normalize()), Con.Cam.Direction.UnitVector()*pro.Normalize()),
        atan2(right * Con.Cam.Direction.UnitVector().CrossProduct(bro.Normalize()), Con.Cam.Direction.UnitVector()*bro.Normalize() ));
        mvprintw(s++, 0, "X: %.2f Y: %.2f Z: %.2f", Con.Cam.Position.X, Con.Cam.Position.Y, Con.Cam.Position.Z);
        mvprintw(s++, 0, "q = %.2f", Con.q);
        mvprintw(s++, 0, "p = %.2f", Con.p);
        mvprintw(s++, 0, "cols = %d, rows = %d, c/r = %.2f", cols, rows, (float)cols/rows);
        mvprintw(s++, 0, "frame time = %ld teomaxFPS = %.2f", frameDuration.count(), 1000.0/frameDuration.count());
        mvprintw(s++, 0, "looop time = %ld fixed FPS = %.2f", MS_PER_FRAME.count(), 1000.0/MS_PER_FRAME.count());
        }
        refresh();  // Aggiorna lo schermo
        lock.unlock();
        //key_mutex.unlock();
        frameEnd = std::chrono::high_resolution_clock::now();
        frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

        if (frameDuration < MS_PER_FRAME) {
            std::this_thread::sleep_for(MS_PER_FRAME - frameDuration);
        }
    }
    // Chiudi curses e ripristina il terminale
    endwin();
}

int main() {
    signal(SIGINT, signalHandler);

    std::thread monitor_thread(monitorDevices);
    displayLoop();

    monitor_thread.join();

    std::lock_guard<std::mutex> lock(device_mutex);
    for (auto& [_, ctx] : device_threads) {
        if (ctx.thread.joinable()) ctx.thread.detach();
        libevdev_free(ctx.dev);
        close(ctx.fd);
    }

    return 0;
}