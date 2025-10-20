#pragma once
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;
template <typename T>
class DoubleBuffer {
   public:
    // Define move assignment, should i copy/move over the rest of the data?
    DoubleBuffer(DoubleBuffer&& other) noexcept {
        for (int i = 0; i < 2; ++i) {
            buffers_[i] = std::move(other.buffers_[i]);
        }
    }

    // Define move assignment, should i copy/move over the rest of the data?
    DoubleBuffer& operator=(DoubleBuffer&& other) noexcept {
        if (this != &other) {
            RequestedIndex = other.RequestedIndex;
            ProposedIndex = other.ProposedIndex;
            for (int i = 0; i < 2; ++i) {
                buffers_[i] = std::move(other.buffers_[i]);
            }
        }
        return *this;
    }

    template <typename... Args>
    DoubleBuffer(Args&&... args) : StopFlag(false), buffers_{T(std::forward<Args>(args)...)} {
        buffers_[1] = buffers_[0];
    }

    void WorkerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);
        ProposedIndex = 1 - ProposedIndex;
        cv.wait(lock, [this] { return (RequestedIndex == ProposedIndex) || StopFlag; });

        lock.unlock();
        cv.notify_one();
    }

    void CustomerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);
        RequestedIndex = 1 - RequestedIndex;
        cv.wait(lock, [this] { return (RequestedIndex == ProposedIndex) || StopFlag; });

        lock.unlock();
        cv.notify_one();
    }

    // where the worker writes
    inline T& New() { return buffers_[1 - ProposedIndex]; }

    // where the customer reads
    inline const T& Old() const { return buffers_[ProposedIndex]; }

    // where customer can also read,
    // absolutely unnecessary buut i think it's pretty neat!
    inline const T& Out() const { return buffers_[RequestedIndex]; }

    DoubleBuffer(const DoubleBuffer&) = default;
    // DoubleBuffer(DoubleBuffer&&) noexcept = default;
    DoubleBuffer& operator=(const DoubleBuffer&) = default;
    // DoubleBuffer& operator=(DoubleBuffer&&) noexcept = default;
    inline void Stop() {
        StopFlag = true;
        cv.notify_one();
    }

   private:
    std::condition_variable cv;
    std::mutex mutex_;
    T buffers_[2];
    bool StopFlag;
    int RequestedIndex = 0;
    int ProposedIndex = 0;
};