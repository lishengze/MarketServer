#include "data_receive.h"
#include "../front_server_declare.h"
#include "quark/cxx/assign.h"
#include "pandora/util/time_util.h"
#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../log/log.h"
#include "quote.h"
#include "config/config.h"

#include <chrono>
#include <sstream>

DataReceive::DataReceive(utrade::pandora::io_service_pool& pool, IPackageStation* next_station)
    :ThreadBasePool(pool), IPackageStation(next_station)
{

}

DataReceive::~DataReceive()
{
    if (!test_kline_thread_)
    {
        if (test_kline_thread_->joinable())
        {
            test_kline_thread_->join();
        }
    }

    if (!test_enquiry_thread_)
    {
        if (test_enquiry_thread_->joinable())
        {
            test_enquiry_thread_->join();
        }
    }
}

void DataReceive::launch()
{
    LOG_INFO("DataReceive::launch");
    
    init_grpc_interface();

    // init_statistic_thread();

    // test_main();    
}

void DataReceive::init_grpc_interface()
{
    HubInterface::set_callback(this);
    HubInterface::start(CONFIG->get_file_name());
}


void DataReceive::test_main()
{
    // test_rsp_package();

    if (is_test_enquiry)
    {
        test_enquiry_thread_ = std::make_shared<std::thread>(&DataReceive::test_enquiry, this);
    }

    if (is_test_kline)
    {
        test_kline_thread_ = std::make_shared<std::thread>(&DataReceive::test_kline_data, this);
    }
    

    if (is_test_maxmin_kline)
    {
        max_min_kline_info_60.px_high = MIN_DOUBLE;
        max_min_kline_info_60.px_low = MAX_DOUBLE;
        max_min_kline_info_60.symbol = test_kline_symbol;

        max_min_kline_info_3600.px_high = MIN_DOUBLE;
        max_min_kline_info_3600.px_low = MAX_DOUBLE;
        max_min_kline_info_3600.symbol = test_kline_symbol;        
    }
}

void DataReceive::test_kline_data()
{
    std::vector<string> symbol_list{"BTC_USDT", "XRP_USDT"};
    int frequency_secs = 60;
    type_tick end_time_secs = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
    end_time_secs = mod_secs(end_time_secs, frequency_secs);

    int test_time_numb = 60 * 2;

    double test_max = 100;
    double test_min = 10;
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(test_min, test_max);    
    std::uniform_real_distribution<> offset(1,10);

    for (int i = 0; i < test_time_numb; ++i)
    {
        type_tick cur_time = end_time_secs - (test_time_numb-i) * frequency_secs;

        double open = dis(gen);
        double close = dis(gen);
        double high = std::max(open, close) + offset(gen);
        double low = std::min(open, close) - offset(gen);
        double volume = dis(gen) * 5;

        for (auto symbol:symbol_list)
        {
            KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

            std::vector<KlineData> vec_kline{*kline_data};

            handle_kline_data("", symbol.c_str(), -1, vec_kline);

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(60));

        type_tick cur_time = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
        end_time_secs = mod_secs(end_time_secs, frequency_secs);

        double open = dis(gen);
        double close = dis(gen);
        double high = std::max(open, close) + offset(gen);
        double low = std::min(open, close) - offset(gen);
        double volume = dis(gen) * 5;

        for (auto symbol:symbol_list)
        {

            KlineData* kline_data = new KlineData(symbol, cur_time, open, high, low, close, volume);

            std::vector<KlineData> vec_kline{*kline_data};

            handle_kline_data("", symbol.c_str(), -1, vec_kline);   

            std::stringstream s_obj;
            s_obj << "Update: " <<  get_sec_time_str(cur_time) << " " << symbol << ", "
                << "open: " << open << ", high: " << high << ", "
                << "low: " << low << ", close: " << volume << "\n";       

            LOG_DEBUG(s_obj.str());        
        }         
    }
}

void DataReceive::test_rsp_package()
{
    while(true)
    {
        // PackagePtr package_new = CreateField<>;

        // auto p_rtn_depth = GET_NON_CONST_FIELD(package_new, CUTRtnDepthField);
        // package_new->prepare_response(UT_FID_RtnDepth, ID_MANAGER->get_id());

        // assign(p_rtn_depth->ExchangeID, "HUOBI");
        // assign(p_rtn_depth->LocalTime, utrade::pandora::NanoTimeStr());

        // deliver_response(package_new);  

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void DataReceive::test_enquiry()
{
    std::vector<string> symbol_list{"BTC_USDT", "XRP_USDT"};

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        SDepthData test_raw_depth;

        test_raw_depth.tick = utrade::pandora::NanoTime() / (1000 * 1000 * 1000);
        test_raw_depth.ask_length = 50;
        test_raw_depth.bid_length = 50;

        test_raw_depth.is_raw = true;
        for (int i = 0; i < test_raw_depth.ask_length; ++i)
        {
            test_raw_depth.asks[i].price = 2000 - i * 10;
            test_raw_depth.asks[i].volume = 5;
        }
        for (int i = 0; i < test_raw_depth.bid_length; ++i)
        {
            test_raw_depth.bids[i].price = 1000 - i * 10;
            test_raw_depth.bids[i].volume = 5;
        }        

        for (auto symbol:symbol_list)
        {
            assign(test_raw_depth.symbol, symbol);
            handle_raw_depth("", symbol.c_str(), test_raw_depth);              
        }         
    }
}

void DataReceive::release()
{

}

void DataReceive::request_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataReceive::handle_request_message, this, package));
}

void DataReceive::response_message(PackagePtr package)
{
    get_io_service().post(std::bind(&DataReceive::handle_response_message, this, package));
}

void DataReceive::handle_request_message(PackagePtr package)
{
    switch (package->Tid())
    {
        case UT_FID_ReqKLineData:
            return;
        default:
            break;
    }
}

void DataReceive::handle_response_message(PackagePtr package)
{

}

// 深度数据（推送）
int DataReceive::on_depth(const char* exchange, const char* symbol, const SDepthData& depth)
{   
    // return 1;
    if (is_test_depth)
    {
        return -1;
    }    
    // get_io_service().post(std::bind(&DataReceive::handle_depth_data, this, exchange, symbol, std::ref(depth)));

    // std::stringstream stream_obj;
    // stream_obj  << "[Depth] " << exchange<< " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length;
    // LOG_INFO(stream_obj.str());


    handle_depth_data(exchange, symbol, depth);

    return 1;
}

// K线数据（推送）
int DataReceive::on_kline(const char* exchange, const char* symbol, type_resolution resolution, const vector<KlineData>& klines)
{   
    if (is_test_kline)
    {
        return -1;
    }

    // get_io_service().post(std::bind(&DataReceive::handle_kline_data, this, exchange, symbol, resolution, std::ref(klines)));

    handle_kline_data(exchange, symbol, resolution, klines);

    return 1;
}

// 原始深度数据推送
// int DataReceive::on_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth)
// {
//     return 1;
//     if (is_test_enquiry)
//     {
//         return 1;
//     }
//     get_io_service().post(std::bind(&DataReceive::handle_raw_depth, this, exchange, symbol, depth));
//     return 1;
// }

int DataReceive::on_trade(const char* exchange, const char* symbol, const Trade& trade) 
{ 

    // get_io_service().post(std::bind(&DataReceive::handle_trade_data, this, exchange, symbol, std::ref(trade)));

    handle_trade_data(exchange, symbol, trade);

    return 0; 
}

void DataReceive::handle_raw_depth(const char* exchange, const char* symbol, const SDepthData& depth)
{    
    if (strlen(symbol) == 0) 
    {
        LOG_ERROR("DataReceive::handle_raw_depth symbol is null! \n");
        return;
    }

    if (strcmp(exchange, MIX_EXCHANGE_NAME) != 0)
    {
        // 只处理聚合数据;
        return;
    }

    LOG->record_input_info(string("raw_depth_") + string(exchange) + "_" + string(symbol), depth);
    
    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    if (package)
    {
        SDepthDataPtr pSDepthData = GetField<SDepthData>(package);

        if (pSDepthData)
        {
            pSDepthData->is_raw = true;
            deliver_response(package);
        }
        else
        {
            LOG_ERROR("DataReceive::handle_raw_depth GetField Failed!");
        }
    }
    else
    {
        LOG_ERROR("DataReceive::handle_raw_depth GetNewSDepthDataPackage Failed!");
    }
}

void DataReceive::handle_depth_data(const char* exchange, const char* symbol, const SDepthData& depth)
{    
    if (strlen(symbol) == 0) 
    {
        LOG_ERROR("DataReceive::handle_depth_data symbol is null! \n");
        return;
    }

    if (strcmp(exchange, MIX_EXCHANGE_NAME) != 0)
    {
        // 只处理聚合数据;
        return;
    }

    LOG->record_input_info(string("depth_") + string(exchange) + "_" + string(symbol), depth);

    // if (strcmp(symbol, "BTC_USDT") == 0)
    // {              
    //     std::stringstream stream_obj;
    //     stream_obj  << "[Depth] " << depth.exchange << " " << depth.symbol << " " << depth.ask_length << " " << depth.bid_length << "\n";
    //     LOG_DEBUG(stream_obj.str());        
    // }    

    PackagePtr package = GetNewSDepthDataPackage(depth, ID_MANAGER->get_id());

    if (package)
    {
        SDepthDataPtr pSDepthData = GetField<SDepthData>(package);
        if (pSDepthData)
        {
            pSDepthData->is_raw = false;
            deliver_response(package);
        }
        else
        {
            LOG_ERROR("GetField Failed!");
        }
    }
    else
    {
        LOG_ERROR("GetNewSDepthDataPackage Failed!");
    }
}

void DataReceive::handle_kline_data(const char* exchange, const char* c_symbol, type_resolution resolution, const vector<KlineData>& klines)
{
    if (strlen(c_symbol) == 0) 
    {
        // LOG_ERROR ("DataReceive::handle_kline_data symbol is null!");
        return;
    }

    if (strcmp(exchange, MIX_EXCHANGE_NAME) != 0)
    {
        // 只处理聚合数据;
        return;
    }

    LOG->record_input_info(string("kline_") + string(exchange) + "_" + string(c_symbol) + "_" 
                            + std::to_string(resolution) + "_" + std::to_string(klines.size()), 
                            klines);

    string symbol = string(c_symbol);

    for( int i = 0 ; i < klines.size() ; i ++ )
    {
        KlineData kline(klines[i]);

        std::stringstream stream_obj;
        // if (symbol == "BTC_USDT" && klines.size() < 5)
        // {
        //     stream_obj.clear();
        //     stream_obj  << "[K-Kine] SRC " << get_sec_time_str(kline.index)  << ", " << kline.symbol 
        //                 << ",fre: " << resolution << ", "
        //                 << "open: " << kline.px_open.get_value() << ", high: " << kline.px_high.get_value() << ", "
        //                 << "low: " << kline.px_low.get_value() << ", close: " << kline.px_close.get_value();    
        //     LOG_DEBUG(stream_obj.str());        
        // }

        PackagePtr package = CreatePackage<KlineData>(kline);
        if (package)
        {
            package->prepare_response(UT_FID_KlineData, ID_MANAGER->get_id());
            KlineDataPtr pklineData = GetField<KlineData>(package);
            if (pklineData)
            {
                pklineData->frequency_ = resolution;
                deliver_response(package);
            }
            else
            {
                LOG_ERROR("GetField Failed!");
            }

        }   

        // if (strcmp(c_symbol, "BTC_USDT") == 0 && resolution == 60)
        // {
        //     LOG_INFO(stream_obj.str());
        // }

        // if (strcmp(c_symbol, test_kline_symbol.c_str()) == 0)
        // {
        //     if (is_test_maxmin_kline)
        //     {
        //         if (resolution == 60)
        //         {
        //             if (max_min_kline_info_60.px_high < kline.px_high)
        //             {
        //                 max_min_kline_info_60.px_high = kline.px_high;
        //                 max_min_kline_info_60.high_time = kline.index;
        //             }

        //             if (max_min_kline_info_60.px_low > kline.px_low)
        //             {
        //                 max_min_kline_info_60.px_low = kline.px_low;
        //                 max_min_kline_info_60.low_time = kline.index;
        //             }
        //         }

        //         if (resolution == 3600)
        //         {
        //             if (max_min_kline_info_3600.px_high < kline.px_high)
        //             {
        //                 max_min_kline_info_3600.px_high = kline.px_high;
        //                 max_min_kline_info_3600.high_time = kline.index;
        //             }

        //             if (max_min_kline_info_3600.px_low > kline.px_low)
        //             {
        //                 max_min_kline_info_3600.px_low = kline.px_low;
        //                 max_min_kline_info_3600.low_time = kline.index;
        //             }                    
        //         }

        //     }
        // }
                
    }   
}

void DataReceive::handle_trade_data(const char* exchange, const char* symbol, const Trade& trade)
{
    try
    {
        if (strlen(symbol) == 0) 
        {
            // LOG_ERROR ("DataReceive::handle_trade_data symbol is null!");
            return;
        }        

        if (strcmp(exchange, MIX_EXCHANGE_NAME) != 0)
        {
            // 只处理聚合数据;
            return;
        }

        // if (strcmp(symbol, "BTC_USDT") == 0)
        // {              
        //     std::stringstream stream_obj;
        //     stream_obj << "[trade] " << utrade::pandora::ToSecondStr(trade.time) << " " << exchange << " " << symbol << " "
        //                << trade.price.get_value() << ", " << trade.volume.get_value();
        //     LOG_DEBUG(stream_obj.str());        
        // }

        LOG->record_input_info(string("trade_") + string(exchange) + "_" + string(symbol), trade);

        PackagePtr package = CreatePackage<TradeData>(symbol, trade.exchange, trade.time/1000000000, trade.price, trade.volume);

        if (package)
        {
            package->prepare_response(UT_FID_TradeData, ID_MANAGER->get_id());
            deliver_response(package);
        }
        else
        {
            LOG_ERROR("CreatePackage<TradeData> Failed!");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
    catch(...)
    {
        LOG_ERROR("Unknown Exception!");
    }    
}
