#pragma once

#include <glog/logging.h>

#include <chrono>
#include <string>
#include <vector>

void log_function_calls_per_second();

class ScopedTimer
{
public:
    ScopedTimer(const std::string& name = "Operation") 
        : name_(name), start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start_;
        LOG(INFO) << name_ << " took " << duration.count() << " milliseconds.\n";
    }

private:
    std::string name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

template<typename T>
class CircularBuffer
{
public:
    CircularBuffer(size_t size)
     : buffer_(size)
     , head_(0)
     , tail_(0)
     , full_(false) {}

    void push(T value)
    {
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % buffer_.size();
        full_ = tail_ == head_;

        if (full_)
        {
            head_ = (head_ + 1) % buffer_.size(); // Overwrite the oldest data
        }
    }

    T pop()
    {
        if (is_empty())
        {
            throw std::runtime_error("Empty buffer");
        }

        T value = buffer_[head_];
        head_ = (head_ + 1) % buffer_.size();
        full_ = false;

        return value;
    }

    bool is_empty() const
    {
        return (!full_ && (head_ == tail_));
    }

    bool is_full() const
    {
        return full_;
    }

private:
    std::vector<T> buffer_;
    size_t head_;
    size_t tail_;
    bool full_;
};

