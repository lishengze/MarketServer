#include "tools.h"
#include "../front_server_declare.h"
#include "pandora/util/json.hpp"
#include "../log/log.h"

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
    PackagePtr package =PackagePtr{new Package{}};

    package->SetPackageID(package_id);

    CREATE_FIELD(package, SDepthData);

    SDepthData* pSDepthData = GET_NON_CONST_FIELD(package, SDepthData);

    copy_sdepthdata(pSDepthData, &depth);

    return package;
}

PackagePtr GetNewKlineDataPackage(const KlineData& ori_kline_data, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};
    package->SetPackageID(package_id);
    CREATE_FIELD(package, KlineData);

    KlineData* pklineData = GET_NON_CONST_FIELD(package, KlineData);

    copy_klinedata(pklineData, &ori_kline_data);

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
        depth_level_atom[1] = depth.asks[i].volume.get_value();
        asks[i] = depth_level_atom;
    }
    json_data["asks"] = asks;

    nlohmann::json bids;
    for (int i = 0; i < depth.bid_length; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = depth.bids[i].price.get_value();
        depth_level_atom[1] = depth.bids[i].volume.get_value();
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

string SymbolsToJsonStr(SymbolData& symbol_data, string type)
{
    std::set<std::string>& symbols = symbol_data.get_symbols();

    nlohmann::json json_data;
    nlohmann::json symbol_json;

    int i = 0;
    for (string symbol:symbols)
    {
        symbol_json[i++] = symbol;
    }
    json_data["symbol"] = symbol_json;    

    json_data["type"] = type;

    return json_data.dump();
}

string SymbolsToJsonStr(std::set<std::string>& symbols, string type)
{
    nlohmann::json json_data;
    nlohmann::json symbol_json;

    int i = 0;
    for (string symbol:symbols)
    {
        symbol_json[i++] = symbol;
    }
    json_data["symbol"] = symbol_json;    

    json_data["type"] = type;

    return json_data.dump();
}

string EnhancedDepthDataToJsonStr(EnhancedDepthData& en_data, string type)
{
    string result;
    nlohmann::json json_data;
    json_data["symbol"] = en_data.depth_data_.symbol;
    json_data["exchange"] = en_data.depth_data_.exchange;
    json_data["tick"] = en_data.depth_data_.tick;
    json_data["seqno"] = en_data.depth_data_.seqno;
    json_data["ask_length"] = en_data.depth_data_.ask_length;
    json_data["bid_length"] = en_data.depth_data_.bid_length;   

    nlohmann::json asks_json;
    for (int i = 0; i < en_data.depth_data_.ask_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = en_data.depth_data_.asks[i].price.get_value();
        depth_level_atom[1] = en_data.depth_data_.asks[i].volume.get_value();
        depth_level_atom[2] = en_data.ask_accumulated_volume_[i];
        asks_json[i] = depth_level_atom;
    }
    json_data["asks"] = asks_json;

    nlohmann::json bids_json;
    for (int i = 0; i < en_data.depth_data_.bid_length && i < DEPCH_LEVEL_COUNT; ++i)
    {
        nlohmann::json depth_level_atom;
        depth_level_atom[0] = en_data.depth_data_.bids[i].price.get_value();
        depth_level_atom[1] = en_data.depth_data_.bids[i].volume.get_value();
        depth_level_atom[2] = en_data.bid_accumulated_volume_[i];
        bids_json[i] = depth_level_atom;
    }
    json_data["bids"] = bids_json;
    json_data["type"] = type;

    result = json_data.dump(); 
    
    return result;
}

std::vector<AtomKlineDataPtr>& compute_target_kline_data(std::vector< KlineData*>& kline_data, int frequency)
{
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

PackagePtr GetNewReqKLineDataPackage(string symbol, type_tick start_time, type_tick end_time,  int frequency, int package_id, 
                                    HttpResponse * response, HttpRequest *request)
{
    PackagePtr package{nullptr};
    try
    {    
        package = PackagePtr{new Package{}};
        package->SetPackageID(package_id);

        CREATE_FIELD(package, ReqKLineData);

        ReqKLineData* p_req_kline_data = GET_NON_CONST_FIELD(package, ReqKLineData);

        p_req_kline_data->http_response_ = response;
        p_req_kline_data->http_request_ = request;

        p_req_kline_data->symbol_ = symbol;
        p_req_kline_data->start_time_ = start_time;
        p_req_kline_data->end_time_ = end_time;
        p_req_kline_data->frequency_ = frequency;

        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewSymbolDataPackage: " << e.what() << '\n';
    }
    
    return package;   
}

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<AtomKlineDataPtr>& main_data, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        CREATE_FIELD(package, RspKLineData);

        RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

        p_rsp_kline_data->comm_type = pReqKlineData->comm_type;
        p_rsp_kline_data->http_request_ = pReqKlineData->http_request_;
        p_rsp_kline_data->http_response_ = pReqKlineData->http_response_;
        p_rsp_kline_data->websocket_ = pReqKlineData->websocket_;

        p_rsp_kline_data->symbol_ = pReqKlineData->symbol_;
        p_rsp_kline_data->start_time_ = pReqKlineData->start_time_;
        p_rsp_kline_data->end_time_ = pReqKlineData->end_time_;
        p_rsp_kline_data->frequency_ = pReqKlineData->frequency_;

        p_rsp_kline_data->kline_data_vec_ = main_data;

        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewSymbolDataPackage: " << e.what() << '\n';
    }
    
    return package;        
}

string RspKlinDataToJsonStr(RspKLineData& rsp_kline_data, string type)
{
    try
    {
        string result;
        nlohmann::json json_data;        
        json_data["type"] = type;
        json_data["symbol"] = rsp_kline_data.symbol_;
        json_data["start_time"] = rsp_kline_data.start_time_;
        json_data["end_time"] = rsp_kline_data.end_time_;
        json_data["frequency"] = rsp_kline_data.frequency_;

        int i = 0;
        nlohmann::json detail_data;
        for (AtomKlineDataPtr atom_data:rsp_kline_data.kline_data_vec_)
        {
            nlohmann::json tmp_json;
            tmp_json["open"] = atom_data->open_;
            tmp_json["high"] = atom_data->high_;
            tmp_json["low"] = atom_data->low_;
            tmp_json["close"] = atom_data->close_;
            tmp_json["volume"] = atom_data->volume_;
            tmp_json["tick"] = atom_data->tick_;
            detail_data[i++] = tmp_json;
        }
        json_data["data"] = detail_data;
        return json_data.dump();
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] RspKlinDataToJsonStr: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    
}
