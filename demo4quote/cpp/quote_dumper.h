#pragma once

#include "stream_engine_define.h"


struct QuotePackage
{
    char ver[32];
    type_tick tick;
    bool is_snap;
    SDepthQuote quote;
};
#define QUOTEPACKAGE_VER "1.0"

class QuoteDumper
{
public:
    QuoteDumper(){}

    void start() {
        checker_loop_ = new std::thread(&QuoteDumper::_dump_thread, this);
    }

    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) 
    {
        QuotePackage tmp;
        strcpy(tmp.ver, QUOTEPACKAGE_VER);
        tmp.tick = get_miliseconds();
        tmp.is_snap = true;
        tmp.quote = quote;
        std::unique_lock<std::mutex> inner_lock{ mutex_pkgs_ };
        pkgs_.push_back(tmp);
    }

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote) 
    {
        QuotePackage tmp;
        strcpy(tmp.ver, QUOTEPACKAGE_VER);
        tmp.tick = get_miliseconds();
        tmp.is_snap = false;
        tmp.quote = quote;
        std::unique_lock<std::mutex> inner_lock{ mutex_pkgs_ };
        pkgs_.push_back(tmp);
    }

private:

    std::thread* checker_loop_ = nullptr;
    void _dump_thread()
    {
        FILE *f = fopen("dump.dat", "wb");

        vector<QuotePackage> pkgs;
        while( true )
        {
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_pkgs_ };
                pkgs.swap(pkgs_);
            }

            for( const auto& v: pkgs ) {
                fwrite(&v, sizeof(v), 1, f);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        fclose(f);
    }

    mutable std::mutex mutex_pkgs_;
    vector<QuotePackage> pkgs_;
};