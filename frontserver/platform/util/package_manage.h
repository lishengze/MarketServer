
#pragma once

#include "pandora/package/package.h"
#include "hub_struct.h"
#include "../data_structure/comm_data.h"
#include "../front_server_declare.h"
#include <random>

PackagePtr GetNewRspRiskCtrledDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id);

PackagePtr GetNewRspSymbolListDataPackage(std::set<string> symbol, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<AtomKlineDataPtr>& main_data, int package_id);

PackagePtr GetReqEnquiryPackage(string symbol, double volume, double amount, HttpResponseThreadSafePtr res);

PackagePtr GetRspEnquiryPackage(string symbol, double price, HttpResponseThreadSafePtr res);
