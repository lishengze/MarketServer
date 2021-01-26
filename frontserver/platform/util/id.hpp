#pragma once

#include <atomic>

using ID_TYPE = long long;

class ID
{
    public:
        ID_TYPE get_id() {return id_++;}

    private:
        std::atomic_ullong      id_{1};
};