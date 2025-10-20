#pragma once
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;
template <typename T>
class MilkMan {
   public:
    // Define move assignment, should i copy/move over the rest of the data?
    MilkMan(MilkMan&& other) noexcept {
        for (int i = 0; i < 3; ++i) {
            buffers_[i] = std::move(other.buffers_[i]);
        }
    }

    // Define move assignment, should i copy/move over the rest of the data?
    MilkMan& operator=(MilkMan&& other) noexcept {
        if (this != &other) {
            for (int i = 0; i < 3; ++i) {
                buffers_[i] = std::move(other.buffers_[i]);
            }
        }
        return *this;
    }

    template <typename... Args>
    MilkMan(Args&&... args) : buffers_{T(std::forward<Args>(args)...)}, StopFlag(false) {
        buffers_[1] = buffers_[0];
        buffers_[2] = buffers_[0];
    }

    void WorkerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);
        FreshIndex = WorkerWritingIndex;

        if (WorkerReadingIndex == CustomerReadingIndex) {  // customer has come
            WorkerWritingIndex = UsedIndex;
        } else {  // customer has not come
            WorkerWritingIndex = WorkerReadingIndex;
        }
        // WorkerWritingIndex = (WorkerReadingIndex == CustomerReadingIndex) ?? UsedIndex :
        // WorkerReadingIndex;
        WorkerReadingIndex = FreshIndex;
        lock.unlock();
        cv.notify_one();
    }

    void CustomerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);

        auto timeout = std::chrono::steady_clock::now() + 4s;
        cv.wait(lock, [this] { return (WorkerReadingIndex != CustomerReadingIndex) || StopFlag; });
        UsedIndex = CustomerReadingIndex;
        CustomerReadingIndex = FreshIndex;
        lock.unlock();
    }

    inline const T& Out() const { return buffers_[CustomerReadingIndex]; }
    inline T& New() { return buffers_[WorkerWritingIndex]; }
    inline const T& Old() const { return buffers_[WorkerReadingIndex]; }

    inline void Stop() {
        StopFlag = true;
        cv.notify_one();
    }

   private:
    std::condition_variable cv;
    std::mutex mutex_;
    bool StopFlag;
    T buffers_[3];
    int CustomerReadingIndex = 0;
    int FreshIndex = 0;
    int UsedIndex = 2;
    int WorkerWritingIndex = 1;
    int WorkerReadingIndex = 0;
};