
#pragma once

#include "pandora/package/package.h"
#include "hub_struct.h"
#include "../data_structure/comm_data.h"
#include "../front_server_declare.h"
#include <random>

// template<class user_class>
// PackagePtr CreatePackage(Args&&... args) 
// {
//     auto data_ptr = boost::make_shared<user_class>(std::forward<Args>(args)...);
// }
PackagePtr GetReqSymbolListDataPackage(ID_TYPE socket_id, COMM_TYPE socket_type, int package_id, bool is_cancel_request=false);

PackagePtr GetReqRiskCtrledDepthDataPackage(string& symbol, ID_TYPE socket_id, int package_id, bool is_cancel_request=false);

PackagePtr GetNewRspRiskCtrledDepthDataPackage(const SDepthData& depth, ID_TYPE socket_id, COMM_TYPE socket_type, int package_id);

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id);

PackagePtr GetNewRspSymbolListDataPackage(std::set<string> symbol, ID_TYPE socket_id, COMM_TYPE socket_type, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<KlineDataPtr>& main_data, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, KlineDataPtr& update_kline_data, int package_id);

PackagePtr GetReqEnquiryPackage(string symbol, double volume, double amount, int type, ID_TYPE socket_id, COMM_TYPE socket_type);

PackagePtr GetRspEnquiryPackage(string symbol, double price, ID_TYPE socket_id, COMM_TYPE socket_type);

PackagePtr GetRspErrMsgPackage(string err_msg, int err_id, ID_TYPE socket_id, COMM_TYPE socket_type);


