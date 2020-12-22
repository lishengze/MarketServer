#pragma once

#include <mutex>
#include "../front_server_declare.h"
#include "../data_structure/data_struct.h"

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

    void response_src_sdepth_package(PackagePtr package);
    
    void response_new_symbol(string symbol);

    using EnhancedDepthDataPackagePtr = PackagePtr; 

private:
    map<string, EnhancedDepthDataPackagePtr>    depth_data_;
    std::mutex                                  depth_data_mutex_;

    DataProcessPtr                              process_engine_;    
};

FORWARD_DECLARE_PTR(DepthProces);