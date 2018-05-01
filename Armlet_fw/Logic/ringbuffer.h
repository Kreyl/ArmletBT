#pragma once

#include <cstddef>

template<class T, size_t N>
class RingBuffer
{
public:
    RingBuffer()
        : data_(),
          head_(0),
          tail_(0)
    {
    }

    size_t size()
    {
        return (N + head_ - tail_) % N;
    }

    bool pop(T &entry)
    {
        if (size() == 0) {
            return false;
        }

        entry = data_[tail_];
        tail_ = (tail_ + 1) % N;

        return true;
    }

    bool push(const T &entry)
    {
        if (size() == N - 1) {
            return false;
        }

        data_[head_] = entry;
        head_ = (head_ + 1) % N;

        return true;
    }

private:
    T data_[N];

    size_t head_;
    size_t tail_;
};
