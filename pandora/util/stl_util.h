// @Copyright (c) 2019, Reserved Co. All Rights Reserved
// @Author:   cao.ning
// @Date:     2019/09/20
// @Brief:

#ifndef __STL_UTIL__
#define __STL_UTIL__

#include <stdint.h>
#include <deque>
#include <mutex>

// @Class:   LockedDeque
// @Author:  cao.ning
// @Date:    2019/09/20
// @Brief:
template <typename T, typename LockT = std::mutex>
class LockedDeque {
    using this_type = LockedDeque;
public:
    LockedDeque() = default;
    ~LockedDeque() = default;
    //
    bool empty() const {
        std::lock_guard<LockT> lg(lock_);
        return deque_.empty();
    }
    size_t size() const {
        std::lock_guard<LockT> lg(lock_);
        return deque_.size();
    }
    void clear() {
        std::lock_guard<LockT> lg(lock_);
        deque_.clear();
    }
    void push_back(const T& t) {
        std::lock_guard<LockT> lg(lock_);
        deque_.push_back(t);
    }
    void push_back(T&& t) {
        std::lock_guard<LockT> lg(lock_);
        deque_.push_back(std::forward<T>(t));
    }
    void push_front(const T& t) {
        std::lock_guard<LockT> lg(lock_);
        deque_.push_front(t);
    }
    void push_front(T&& t) {
        std::lock_guard<LockT> lg(lock_);
        deque_.push_front(std::forward<T>(t));
    }
    void pop_front() {
        std::lock_guard<LockT> lg(lock_);
        deque_.pop_front();
    }
    void pop_back() {
        std::lock_guard<LockT> lg(lock_);
        deque_.pop_back();
    }
    inline LockT& get_lock() { return lock_; }
    inline typename std::deque<T>& get_deque() { return deque_; }
    inline const typename std::deque<T>& get_deque() const { return deque_; }
private:
    mutable LockT lock_;
    std::deque<T> deque_;
};

#endif // __STL_UTIL__