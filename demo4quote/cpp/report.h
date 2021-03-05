#pragma once

#include "base/cpp/basic.h"

class Report
{
public:
    void start();

private:    
    std::thread* loop_ = nullptr;
    std::atomic<bool> thread_run_;

    void _looping();
};