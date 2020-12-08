#pragma once

#include "pandora/package/package.h"
#include "hub_struct.h"
#include "../data_structure/data_struct.h"
#include "../front_server_declare.h"

void copy_sdepthdata(SDepthData* des, const SDepthData* src);

void copy_klinedata(KlineData* des, const KlineData* src);

void copy_enhanced_data(EnhancedDepthData* des, const EnhancedDepthData* src);

PackagePtr GetNewEnhancedDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id);

PackagePtr GetNewSymbolDataPackage(std::set<string> symbol, int package_id);

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<AtomKlineDataPtr>& main_data, int package_id);

string SDepthDataToJsonStr(const SDepthData& depth);

string SymbolsToJsonStr(SymbolData& symbol_data, string type=SYMBOL_UPDATE);

string SymbolsToJsonStr(std::set<std::string>& symbols, string type=SYMBOL_LIST);

string EnhancedDepthDataToJsonStr(EnhancedDepthData& en_data, string type=MARKET_DATA_UPDATE);

std::vector<AtomKlineDataPtr>& compute_target_kline_data(std::vector< KlineData*>& kline_data, int frequency);

