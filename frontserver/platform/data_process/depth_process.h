#pragma once

#include <mutex>
#include "../front_server_declare.h"
#include "../data_structure/comm_data.h"

using std::map;
using std::vector;


class DataProcess;
class DepthProces
{
public:
    using DataProcessPtr = boost::shared_ptr<DataProcess>;

    DepthProces();

    virtual ~DepthProces() {}

    void init_process_engine(DataProcessPtr process_engine);

    void request_depth_package(PackagePtr package);

    void request_symbol_package(PackagePtr package);

    void request_enquiry_package(PackagePtr package);

    void response_src_sdepth_package(PackagePtr package);
    
    void response_new_symbol(string symbol);

    double compute_enquiry_price(SDepthDataPtr depth_data, int type, double volume, double amount);

    using RspRiskCtrledDepthDataPackagePtr = PackagePtr; 


private:
    map<string, RspRiskCtrledDepthDataPackagePtr>    depth_data_;
    map<string, SDepthDataPtr>                       raw_depth_data_;

    std::mutex                                       raw_depth_data_mutex_;
    std::mutex                                       depth_data_mutex_;

    DataProcessPtr                                   process_engine_;    
};

FORWARD_DECLARE_PTR(DepthProces);