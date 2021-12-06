#pragma once

#include "global_declare.h"

#include "struct_define.h"

class KlineProcessor
{
public:

    KlineProcessor();

    ~KlineProcessor();

    void process(vector<KlineData>& src);
};