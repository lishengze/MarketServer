#include "package_manage.h"
#include "tools.h"

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

PackagePtr GetNewRspRiskCtrledDepthDataPackage(const SDepthData& depth, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};

    try
    {        
        package->SetPackageID(package_id);

        CREATE_FIELD(package, RspRiskCtrledDepthData);

        RspRiskCtrledDepthData* p_enhanced_depth_data = GET_NON_CONST_FIELD(package, RspRiskCtrledDepthData);

        p_enhanced_depth_data->init(&depth);

        package->prepare_response(UT_FID_RspRiskCtrledDepthData, package->PackageID());        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return package;    
}

PackagePtr GetNewRspSymbolListDataPackage(std::set<string> symbols, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        CREATE_FIELD(package, RspSymbolListData);

        RspSymbolListData* p_symbol_data = GET_NON_CONST_FIELD(package, RspSymbolListData);

        p_symbol_data->set_symbols(symbols);
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;       
}

PackagePtr GetReqEnquiryPackage(string symbol, double volume, double amount, HttpResponseThreadSafePtr res)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        ID_TYPE id = ID_MANAGER->get_id();
        package->SetPackageID(id);
        package->prepare_request(UT_FID_ReqEnquiry, id);

        CREATE_FIELD(package, ReqEnquiry);

        ReqEnquiry* p_req_enquiry = GET_NON_CONST_FIELD(package, ReqEnquiry);

        p_req_enquiry->set(symbol, volume, amount, res);        
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
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

        assign(p_rsp_kline_data->comm_type, pReqKlineData->comm_type);
        assign(p_rsp_kline_data->http_response_, pReqKlineData->http_response_);
        assign(p_rsp_kline_data->websocket_, pReqKlineData->websocket_);

        assign(p_rsp_kline_data->symbol_, pReqKlineData->symbol_);
        assign(p_rsp_kline_data->start_time_, pReqKlineData->start_time_);
        assign(p_rsp_kline_data->end_time_, pReqKlineData->end_time_);
        assign(p_rsp_kline_data->frequency_, pReqKlineData->frequency_);
        assign(p_rsp_kline_data->ws_id_, pReqKlineData->ws_id_);
        assign(p_rsp_kline_data->data_count_, main_data.size());

        for (AtomKlineDataPtr atom_kline:main_data)
        {
            p_rsp_kline_data->kline_data_vec_.emplace_back(atom_kline);
        }

        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;        
}

PackagePtr GetRspEnquiryPackage(string symbol, double price, HttpResponseThreadSafePtr res)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        ID_TYPE id = ID_MANAGER->get_id();
        package->SetPackageID(id);
        package->prepare_response(UT_FID_RspEnquiry, id);

        CREATE_FIELD(package, RspEnquiry);

        RspEnquiry* p_rsp_enquiry_data = GET_NON_CONST_FIELD(package, RspEnquiry);

        p_rsp_enquiry_data->set(symbol, price, res);
    
        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;      
}