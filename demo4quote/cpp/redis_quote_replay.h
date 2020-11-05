#pragma once

#include "quote_dumper.h"

class RedisQuoteReplay
{
public:
    RedisQuoteReplay(){
    }
    ~RedisQuoteReplay(){        
        if (checker_loop_) {
            if (checker_loop_->joinable()) {
                checker_loop_->join();
            }
            delete checker_loop_;
        }
    }

    void set_engine(QuoteSourceInterface* ptr) { engine_interface_ = ptr; }

    void start(int ratio = 2, const string& filepath = "dump.dat") {
        filepath_ = filepath;
        ratio_ = ratio;
        checker_loop_ = new std::thread(&RedisQuoteReplay::_load_and_send, this);
    }
private:
    // callback
    QuoteSourceInterface *engine_interface_ = nullptr;
    // 文件名
    string filepath_;
    // 回放速度
    int ratio_;

    std::thread* checker_loop_ = nullptr;

    void _load_and_send()
    {
        if( ratio_ < 1 ) {
            std::cout << "unsurpport ratio " << ratio_  << std::endl;
            return;
        }

        FILE* f = fopen(filepath_.c_str(), "rb");
        if( f == NULL ) {
            std::cout << "open " << filepath_ << " failed." << std::endl;
            return;
        }

        type_tick start_time = get_miliseconds();
        type_tick first_pkg_time = 0;
        bool exit = false;
        while( !exit ) 
        {
            type_tick now = get_miliseconds();
            if( first_pkg_time == 0 ) {
                first_pkg_time = _send_one(f);
            } else {
                type_tick limit_time = (now-start_time) * ratio_;
                while( true )
                {
                    type_tick pkg_time = _send_one(f);
                    if( pkg_time == 0 ) {
                        exit = true;
                        break;
                    }
                    if( (pkg_time - first_pkg_time) >= limit_time ) {
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        fclose(f);
        std::cout << "exit replay." << std::endl;
        return;
    }

    type_tick _send_one(FILE* f) {
        QuotePackage pkg;
        int s = fread(&pkg, 1, sizeof(pkg), f);
        if( s != sizeof(pkg) ){
            std::cout << "read from " << filepath_ << " failed." << s << std::endl;
            return 0;
        }
        if( pkg.is_snap ) {
            engine_interface_->on_snap(pkg.quote.exchange, pkg.quote.symbol, pkg.quote);
        } else {
            engine_interface_->on_update(pkg.quote.exchange, pkg.quote.symbol, pkg.quote);
        }
        return pkg.tick;
    }
};
