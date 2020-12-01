#include "tools.h"
#include "../front_server_declare.h"
#include "pandora/util/json.hpp"

void copy_sdepthdata(SDepthData* des, const SDepthData* src)
{
    // *des = *src;
    memcpy(des, src, sizeof (SDepthData));
}

void copy_klinedata(KlineData* des, const KlineData* src)
{
    memcpy(des, src, sizeof (KlineData));
}

void copy_enhanced_data(EnhancedDepthData* des, const EnhancedDepthData* src)
{
    memcpy(des, src, sizeof(EnhancedDepthData));
}

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id)
{
    cout << 0 << endl;
    PackagePtr package =PackagePtr{new Package{}};

    cout << 1 << endl;
    package->SetPackageID(package_id);

    cout << 2 << endl;
    CREATE_FIELD(package, SDepthData);

    cout << 3 << endl;
    SDepthData* pSDepthData = GET_NON_CONST_FIELD(package, SDepthData);

    cout << 4 << endl;
    copy_sdepthdata(pSDepthData, &depth);

    cout << 5 << endl;
    return package;
}

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};
    package->SetPackageID(package_id);
    CREATE_FIELD(package, KlineData);

    KlineData* pklineData = GET_NON_CONST_FIELD(package, KlineData);

    copy_klinedata(pklineData, &depth);

    return package;
}

string SDepthDataToJsonStr(const SDepthData& depth)
{
    nlohmann::json json_data;
    json_data["symbol"] = depth.symbol;
    json_data["exchange"] = depth.exchange;
    json_data["tick"] = depth.tick;
    json_data["seqno"] = depth.seqno;
    json_data["ask_length"] = depth.ask_length;
    json_data["bid_length"] = depth.bid_length;   

    nlohmann::json asks;
    for (int i = 0; i < depth.ask_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.asks[i].price.get_value();
        depth_level_atom[1] = depth.asks[i].volume;
        asks[i] = depth_level_atom;
    }
    json_data["asks"] = asks;

    nlohmann::json bids;
    for (int i = 0; i < depth.bid_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.bids[i].price.get_value();
        depth_level_atom[1] = depth.bids[i].volume;
        bids[i] = depth_level_atom;
    }
    json_data["bids"] = bids;

    return json_data.dump(); 
}

PackagePtr GetNewEnhancedDepthDataPackage(const SDepthData& depth, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};

    try
    {        
        package->SetPackageID(package_id);

        CREATE_FIELD(package, EnhancedDepthData);

        EnhancedDepthData* p_enhanced_depth_data = GET_NON_CONST_FIELD(package, EnhancedDepthData);

        p_enhanced_depth_data->init(&depth);

        package->prepare_response(UT_FID_EnhancedDepthData, package->PackageID());        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return package;    
}

PackagePtr GetNewSymbolDataPackage(std::set<string> symbols, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        CREATE_FIELD(package, SymbolData);

        SymbolData* p_symbol_data = GET_NON_CONST_FIELD(package, SymbolData);

        p_symbol_data->set_symbols(symbols);
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewSymbolDataPackage: " << e.what() << '\n';
    }
    
    return package;       
}

string SymbolsToJsonStr(std::set<std::string>& symbols)
{
    nlohmann::json json_data;
    nlohmann::json symbol_json;

    int i = 0;
    for (string symbol:symbols)
    {
        symbol_json[i++] = symbol;
    }
    json_data["symbol"] = symbol_json;    

    return json_data.dump();
}