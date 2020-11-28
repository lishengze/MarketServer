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
    *des = *src;
    memcpy(des, src, sizeof (KlineData));
}

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};
    package->SetPackageID(package_id);

    CREATE_FIELD(package, SDepthData);

    SDepthData* pSDepthData = GET_NON_CONST_FIELD(package, SDepthData);

    copy_sdepthdata(pSDepthData, &depth);

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
        depth_level_atom[0] = depth.asks[0].price.get_value();
        depth_level_atom[1] = depth.asks[0].volume;
        asks[i] = depth_level_atom;
    }
    json_data["asks"] = asks;

    nlohmann::json bids;
    for (int i = 0; i < depth.bid_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.bids[0].price.get_value();
        depth_level_atom[1] = depth.bids[0].volume;
        bids[i] = depth_level_atom;
    }
    json_data["bids"] = bids;

    return json_data.dump(); 
}