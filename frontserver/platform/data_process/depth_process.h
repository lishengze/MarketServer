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

    void request_symbol_list_package(PackagePtr package);

    void request_enquiry_package(PackagePtr package);

    void response_src_sdepth_package(PackagePtr package);
    
    void response_new_symbol(string symbol);

    double compute_enquiry_price(SDepthDataPtr depth_data, int type, double volume, double amount, string& errr_msg, int& err_id);

    void response_updated_depth_data(SDepthDataPtr p_depth_data);

    void checkout_subdepth_connections(ReqRiskCtrledDepthDataPtr p_req);

    void delete_subdepth_connection(string symbol, ID_TYPE socket_id);

    void delete_reqsymbollist_connection(ReqSymbolListDataPtr pReqSymbolListData);

private:
    map<string, SDepthDataPtr>                       depth_data_;
    map<string, SDepthDataPtr>                       raw_depth_data_;

    std::map<ID_TYPE, ReqSymbolListDataPtr>          req_symbol_list_map_;
    std::mutex                                       req_symbol_list_map_mutex_;

    map<string, vector<ReqRiskCtrledDepthDataPtr>>   subdepth_map_;
    std::mutex                                       subdepth_map_mutex_;

    std::mutex                                       subdepth_con_map_mutex_;
    std::map<ID_TYPE, string>                        subdepth_con_map_;  

    std::mutex                                       raw_depth_data_mutex_;
    std::mutex                                       depth_data_mutex_;

    DataProcessPtr                                   process_engine_;    
};

FORWARD_DECLARE_PTR(DepthProces);