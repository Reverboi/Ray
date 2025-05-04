#pragma once
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
extern std::array<bool, 256> key_states;
extern std::mutex key_mutex;

struct DeviceContext {
    int fd;
    libevdev* dev;
    std::thread thread;
    std::string devnode;
    std::string name;
};

extern std::mutex device_mutex;
extern std::map<std::string, DeviceContext> device_threads;

void readDeviceEvents(libevdev* dev, const std::string& name, const std::atomic<bool>& running);

void addDevice(struct udev_device* dev, const std::atomic<bool>& running);

void removeDevice(const std::string& devnode);

void monitorDevices(const std::atomic<bool>& running);