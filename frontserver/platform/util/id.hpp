#pragma once

#include <atomic>

class ID
{
    public:
        long long get_id() {return id_++;}

    private:
        std::atomic_ullong      id_{0};
};