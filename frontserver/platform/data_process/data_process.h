#pragma once

#include "pandora/package/package_station.h"
#include "pandora/util/singleton.hpp"
#include "pandora/util/thread_basepool.h"
#include "../front_server_declare.h"

#include <boost/shared_ptr.hpp>
#include "../data_structure/data_struct.h"

// 用于处理数据: 为 front-server 提供全量或增量的更新;
// 设置缓冲;
class DataProcess:public utrade::pandora::ThreadBasePool, public IPackageStation
{
public:

    DataProcess(utrade::pandora::io_service_pool& pool, IPackageStation* next_station=nullptr);
    virtual ~DataProcess();

    virtual void launch() override;
    virtual void release() override;
            
    virtual void request_message(PackagePtr package) override;
    virtual void response_message(PackagePtr package) override;

    void handle_request_message(PackagePtr package);
    void handle_response_message(PackagePtr package);

    void response_src_sdepth_package(PackagePtr package);

    void response_src_kline_package(PackagePtr package);

    void response_new_symbol(string symbol);

    void request_symbol_data(PackagePtr package);

    void request_kline_package(PackagePtr package);

    void request_depth_data(PackagePtr package);

    PackagePtr get_kline_package(PackagePtr package);

    void store_kline_data(int frequency, KlineData* pkline_data);

    void init_test_kline_data();

    inline std::vector<AtomKlineDataPtr>& compute_target_kline_data(std::vector< KlineData*>& kline_data, int frequency)
    {
        cout << "DataProcess::compute_target_kline_data" << endl;

        std::vector<AtomKlineDataPtr> result;

        cout << "kline_data.size: " << kline_data.size() << endl;

        if (kline_data.size() == 0) return result;

        AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*(kline_data[0]));
        cout << "\ncur_data.tick: " << cur_data->tick_ << endl;
        // AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>();

        cout << "make over" << endl;

        result.push_back(cur_data); 

        double low = MAX_DOUBLE;
        double high = MIN_DOUBLE;

        kline_data.erase(kline_data.begin());

        cout << "kline_data.size: " << kline_data.size() << endl;

        for (KlineData* atom : kline_data)
        {
            cout << "atom.tick " << atom->index << endl;
            low = low > atom->px_low.get_value() ? atom->px_low.get_value() : low;
            high = high < atom->px_high.get_value() ? atom->px_high.get_value():high;

            if (atom->index - (*result.rbegin())->tick_ >= frequency)
            {            
                AtomKlineDataPtr cur_data = boost::make_shared<AtomKlineData>(*atom);
                cur_data->low_ = low;
                cur_data->high_ = high;
                result.push_back(cur_data); 

                low = MAX_DOUBLE;
                high = MIN_DOUBLE;
            }
        }

        cout << "compute over" << endl;

        return result;        
    }

    using EnhancedDepthDataPackagePtr = PackagePtr; 

private:
    std::map<symbol_type, EnhancedDepthDataPackagePtr>                                   depth_data_;
    std::map<symbol_type, std::map<int, std::map<type_tick, KlineDataPtr>>>    kline_data_;
    std::map<symbol_type, std::map<int, KlineDataPtr>>                         cur_kline_data_;

    bool                                                            test_kline_data_{false};

    std::vector<int>                                                frequency_list_;
    int                                                             frequency_numb_{100};   
    int                                                             frequency_base_;           
};

