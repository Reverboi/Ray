#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

class Logger {
   public:
    enum class Level { INFO, WARNING, ERROR, DEBUG };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void enableFileOutput(const std::string& filename) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        file_.open(filename, std::ios::out | std::ios::app);
        fileOutputEnabled_ = file_.is_open();
    }

    void enableConsoleOutput(bool enable) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        consoleOutputEnabled_ = enable;
    }

    // Thread-safe, async log entry
    void log(Level level, const char* file, int line, const std::string& message) {
#ifndef NDEBUG  // <-- Disable logs in release builds
        std::ostringstream oss;
        oss << "[" << timestamp() << "] "
            << "[" << threadId() << "] "
            << "[" << levelToString(level) << "] " << file << ":" << line << " - " << message;

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            logQueue_.emplace(oss.str());
        }

        cv_.notify_one();
#endif
    }

   private:
    Logger() : stopFlag_(false) {
        workerThread_ = std::thread(&Logger::processQueue, this);
    }

    ~Logger() {
        stopFlag_ = true;
        cv_.notify_all();
        if (workerThread_.joinable()) workerThread_.join();

        if (file_.is_open()) file_.close();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string timestamp() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto itt = system_clock::to_time_t(now);
        auto tm = *std::localtime(&itt);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0')
            << ms.count();
        return oss.str();
    }

    std::string threadId() {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }

    std::string levelToString(Level level) {
        switch (level) {
            case Level::INFO:
                return "INFO";
            case Level::WARNING:
                return "WARNING";
            case Level::ERROR:
                return "ERROR";
            case Level::DEBUG:
                return "DEBUG";
            default:
                return "UNKNOWN";
        }
    }

    void processQueue() {
        while (!stopFlag_ || !logQueue_.empty()) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]() { return stopFlag_ || !logQueue_.empty(); });

            while (!logQueue_.empty()) {
                const std::string& entry = logQueue_.front();

                {
                    std::lock_guard<std::mutex> fileLock(fileMutex_);
                    if (consoleOutputEnabled_) std::cout << entry << std::endl;
                    if (fileOutputEnabled_) file_ << entry << std::endl;
                }

                logQueue_.pop();
            }
        }
    }

    std::queue<std::string> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> stopFlag_;
    std::thread workerThread_;

    std::ofstream file_;
    std::mutex fileMutex_;
    bool fileOutputEnabled_ = false;
    bool consoleOutputEnabled_ = false;
};

// Utility for combining message parts
template <typename... Args>
std::string makeLogMessage(Args&&... args) {
    std::ostringstream oss;
    (oss << ... << args);
    return oss.str();
}

// Logging macros
#ifndef NDEBUG
#define LOG_INFO(...) \
    Logger::instance().log(Logger::Level::INFO, __FILE__, __LINE__, makeLogMessage(__VA_ARGS__))
#define LOG_WARN(...) \
    Logger::instance().log(Logger::Level::WARNING, __FILE__, __LINE__, makeLogMessage(__VA_ARGS__))
#define LOG_ERROR(...) \
    Logger::instance().log(Logger::Level::ERROR, __FILE__, __LINE__, makeLogMessage(__VA_ARGS__))
#define LOG_DEBUG(...) \
    Logger::instance().log(Logger::Level::DEBUG, __FILE__, __LINE__, makeLogMessage(__VA_ARGS__))
#else
// In release builds, macros compile to nothing
#define LOG_INFO(...) (void)0
#define LOG_WARN(...) (void)0
#define LOG_ERROR(...) (void)0
#define LOG_DEBUG(...) (void)0
#endif