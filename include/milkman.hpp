#pragma once
#include <condition_variable>
#include <mutex>

template <typename T>
class MilkMan {
   public:
    MilkMan(MilkMan&& other) noexcept {
        for (int i = 0; i < 3; ++i) {
            buffers_[i] = std::move(other.buffers_[i]);
        }
    }

    // Define move assignment
    MilkMan& operator=(MilkMan&& other) noexcept {
        if (this != &other) {
            for (int i = 0; i < 3; ++i) {
                buffers_[i] = std::move(other.buffers_[i]);
            }
        }
        return *this;
    }

    template <typename... Args>
    MilkMan(Args&&... args) : buffers_{T(std::forward<Args>(args)...)} {
        buffers_[1] = buffers_[0];
        buffers_[2] = buffers_[0];
    }

    void WorkerSwap() {
        std::unique_lock<std::mutex> lock(mutex_);
        FreshIndex = WorkerWritingIndex;

        if (WorkerReadingIndex == CustomerReadingIndex) {  // customer has come
            WorkerWritingIndex = UsedIndex;
        } else {  // customer has non come
            WorkerWritingIndex = WorkerReadingIndex;
        }
        // WorkerWritingIndex = (WorkerReadingIndex == CustomerReadingIndex) ?? UsedIndex :
        // WorkerReadingIndex;
        WorkerReadingIndex = FreshIndex;
        lock.unlock();
        cv.notify_one();
    }

    void CustomerSwap() {
        std::lock_guard<std::mutex> lock(mutex_);
        cv.wait(lock, [] { return WorkerReadingIndex != CustomerReadingIndex });
        UsedIndex = CustomerReadingIndex;
        CustomerReadingIndex = FreshIndex;
    }

    inline const T& Out() const { return buffers_[CustomerReadingIndex]; }
    inline T& New() { return buffers_[WorkerWritingIndex]; }
    inline const T& Old() const { return buffers_[WorkerReadingIndex]; }

   private:
    std::condition_variable cv;
    std::mutex mutex_;
    // std::array<T, 3> buffers_;
    T buffers_[3];
    int CustomerReadingIndex = 0;
    int FreshIndex = 0;
    int UsedIndex = 2;
    int WorkerWritingIndex = 1;
    int WorkerReadingIndex = 0;
};