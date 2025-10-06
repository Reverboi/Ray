#pragma once
#include <condition_variable>
#include <mutex>

template <typename T>
class DoubleBuffer {
   public:
    DoubleBuffer(DoubleBuffer&& other) noexcept {
        for (int i = 0; i < 2; ++i) {
            buffers_[i] = std::move(other.buffers_[i]);
        }
    }

    // Define move assignment
    DoubleBuffer& operator=(DoubleBuffer&& other) noexcept {
        if (this != &other) {
            for (int i = 0; i < 2; ++i) {
                buffers_[i] = std::move(other.buffers_[i]);
            }
        }
        return *this;
    }
    template <typename... Args>
    DoubleBuffer(Args&&... args)
        : buffers_(T(std::forward<Args>(args)...), T(std::forward<Args>(args)...)) {}

    void WorkerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);
        ProposedIndex = 1 - ProposedIndex;
        cv.wait(lock, [] { return RequestIndex == ProposedIndex });

        lock.unlock();
        cv.notify_one();
    }

    void CustomerSwap() {
        std::lock_guard<std::mutex> lock(mutex_);
        RequestedIndex = 1 - RequestIndex;
        cv.wait(lock, [] { return RequestIndex == ProposedIndex });

        lock.unlock();
        cv.notify_one();
    }

    inline const T& Out() const {
        return buffers_[RequestIndex];
    }  // absolutely unnecessary buut i think it's pretty neat!
    inline T& New() { return buffers_[1 - ProposedIndex]; }
    inline const T& Old() const { return buffers_[ProposedIndex]; }

    DoubleBuffer(const DoubleBuffer&) = default;
    DoubleBuffer(DoubleBuffer&&) noexcept = default;
    DoubleBuffer& operator=(const DoubleBuffer&) = default;
    DoubleBuffer& operator=(DoubleBuffer&&) noexcept = default;

   private:
    std::condition_variable cv;
    std::mutex mutex_;
    T buffers_[2];
    int RequestIndex = 0;
    int ProposedIndex = 0;
};