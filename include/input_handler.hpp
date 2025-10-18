#pragma once
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <libudev.h>

#include <atomic>
#include <csignal>
#include <map>
#include <mutex>
#include <string>
#include <thread>

class InputHandler {
    struct DeviceContext {
        int fd;
        libevdev* dev;
        std::thread thread;
        std::string devnode;
        std::string name;
    };

    enum KeyState { NotPressed = 0, Pressed = 1, Held = 2 };

    std::map<std::string, DeviceContext> device_threads;

    std::mutex key_mutex;
    std::mutex device_mutex;

    void readDeviceEvents(libevdev* dev, const std::string& name, const std::atomic<bool>& running);

    void addDevice(struct udev_device* dev, const std::atomic<bool>& running);

    void removeDevice(const std::string& devnode);

    void monitorDevices(const std::atomic<bool>& running);

   public:
    InputHandler();
    std::array<KeyState, 256> key_states;
};