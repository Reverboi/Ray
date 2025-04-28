#include <libevdev/libevdev.h>
#include <libudev.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <string>
#include <csignal>
#include <fcntl.h>

//std::map<int, bool> key_states;
std::array<bool, 256> key_states;
std::mutex key_mutex;

struct DeviceContext {
    int fd;
    libevdev* dev;
    std::thread thread;
    std::string devnode;
    std::string name;
};

std::mutex device_mutex;
std::map<std::string, DeviceContext> device_threads;

void readDeviceEvents(libevdev* dev, const std::string& name, const std::atomic<bool>& running){
    struct input_event ev;
    while (running) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);  // non-blocking? sync?
        if (rc == 0 && ev.type == EV_KEY) {
            std::lock_guard<std::mutex> lock(key_mutex);
                key_states[ev.code] = ev.value;
        } else if (rc < 0 && rc != -EAGAIN) {
            break;
        }
    }
}

void addDevice(struct udev_device* dev, const std::atomic<bool>& running) {
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
    ctx.thread = std::thread(readDeviceEvents, evdev, name, std::ref(running));

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

void monitorDevices(const std::atomic<bool>& running) {
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
        addDevice(dev, running);
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
                        addDevice(dev, running);
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