
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

PackagePtr GetNewRspRiskCtrledDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id);

PackagePtr GetNewRspSymbolListDataPackage(std::set<string> symbol, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<KlineDataPtr>& main_data, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, KlineDataPtr& update_kline_data, int package_id);

PackagePtr GetReqEnquiryPackage(string symbol, double volume, double amount, int type, HttpResponseThreadSafePtr res);

PackagePtr GetRspEnquiryPackage(string symbol, double price, HttpResponseThreadSafePtr res);

PackagePtr GetRspErrMsgPackage(string err_msg, int err_id, 
                                HttpResponseThreadSafePtr res=nullptr, 
                                WebsocketClassThreadSafePtr ws=nullptr);


