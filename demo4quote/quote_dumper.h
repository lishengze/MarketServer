#pragma once

#include "stream_engine_define.h"

class QuoteDumper
{
public:
    QuoteDumper(){
        f = fopen("dump.dat", "wb");
    }

    void on_mix_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {
        char v = 1;
        fwrite(&v, 1, 1, f);
        fwrite(&quote, sizeof(quote), 1, f);
    }

    void on_mix_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
        char v = 0;
        fwrite(&v, 1, 1, f);
        fwrite(&quote, sizeof(quote), 1, f);
    }

private:
    FILE* f;
};