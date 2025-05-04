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

struct KeyInfo {
    std::string device_name;
    std::string state;
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
                key_states[ev.code] = {name, ev.value == 1 ? "Pressed" : "Held"};
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

void displayLoop() {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);

    while (running) {
        clear();
        printw("Live Key Monitor (Press Ctrl+C to Quit)\n");

        std::lock_guard<std::mutex> lock(key_mutex);
        if (key_states.empty()) {
            printw("No keys currently pressed.\n");
        } else {
            for (const auto& [code, info] : key_states) {
                printw("Key %d (%s): %s\n", code, info.device_name.c_str(), info.state.c_str());
            }
        }

        refresh();
        usleep(100000);
    }

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
