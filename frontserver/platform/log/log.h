#pragma once

#include <string>
#include <iostream>
#include "pandora/util/thread_safe_singleton.hpp"

using std::string;
using std::cout;
using std::endl;


#define LOG utrade::pandora::ThreadSafeSingleton<Log>::DoubleCheckInstance()

#define LOG_ERROR(msg) LOG->log(msg, "Error")
#define LOG_INFO(msg) LOG->log(msg, "Info")
#define LOG_DEBUG(msg) LOG->log(msg, "Debug")

class Log
{
    public:
        void log(string msg, string flag)
        {
            cout << flag << ": " << msg << endl;
        }
};