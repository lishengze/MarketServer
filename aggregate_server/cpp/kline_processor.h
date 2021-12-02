#pragma once

#include "global_declare.h"

#include "struct_define.h"

class KlilneProcessor
{
public:

    KlilneProcessor();

    ~KlilneProcessor();

    void process(vector<KlineData>& src);
};