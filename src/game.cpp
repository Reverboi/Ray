#include <atomic>
#include <thread>

#include "context.hpp"

std::atomic<bool> running(true);

void signalHandler(int signum) {
    running.store(false, std::memory_order_relaxed);
}

int main() {
    signal(SIGINT, signalHandler);
    Context ContextInstance(running);
    while (running) {
        ContextInstance.Render();
    }

    return 0;
}