#include "package_manage.h"
#include "tools.h"

PackagePtr GetReqSymbolListDataPackage(ID_TYPE socket_id, COMM_TYPE socket_type, int package_id, bool is_cancel_request)
{
    PackagePtr package =PackagePtr{new Package{}};
    try
    {        
        package->SetPackageID(package_id);
        package->prepare_request(UT_FID_ReqSymbolListData, package_id);
        CREATE_FIELD(package, ReqSymbolListData);

        ReqSymbolListData* p_req= GET_NON_CONST_FIELD(package, ReqSymbolListData);

        p_req->set(socket_id, socket_type, is_cancel_request);        
    }
    catch(const std::exception& e)
    {
        std::cerr <<"GetReqRiskCtrledDepthDataPackage " << e.what() << '\n';
    }    
    return package;    
}

PackagePtr GetReqRiskCtrledDepthDataPackage(string& symbol, ID_TYPE socket_id,  int package_id, bool is_cancel_request)
{
    PackagePtr package =PackagePtr{new Package{}};
    try
    {        
        package->SetPackageID(package_id);
        package->prepare_request(UT_FID_ReqRiskCtrledDepthData, package_id);
        CREATE_FIELD(package, ReqRiskCtrledDepthData);

        ReqRiskCtrledDepthData* p_req= GET_NON_CONST_FIELD(package, ReqRiskCtrledDepthData);

        p_req->set(symbol, socket_id, COMM_TYPE::WEBSOCKET, is_cancel_request);        
    }
    catch(const std::exception& e)
    {
        std::cerr <<"GetReqRiskCtrledDepthDataPackage " << e.what() << '\n';
    }    
    return package;
}

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};
    try
    {        
        package->SetPackageID(package_id);
        package->prepare_response(UT_FID_SDepthData, package_id);

        CREATE_FIELD(package, SDepthData);

        SDepthData* pSDepthData = GET_NON_CONST_FIELD(package, SDepthData);

        copy_sdepthdata(pSDepthData, &depth);        
    }
    catch(const std::exception& e)
    {
        std::cerr <<"GetNewSDepthDataPackage " << e.what() << '\n';
    }    
    return package;
}

PackagePtr GetNewKlineDataPackage(const KlineData& ori_kline_data, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};
    try
    {        
        package->SetPackageID(package_id);
        CREATE_FIELD(package, KlineData);

        KlineData* pklineData = GET_NON_CONST_FIELD(package, KlineData);

        copy_klinedata(pklineData, &ori_kline_data);        
    }
    catch(const std::exception& e)
    {
        std::cerr <<"GetNewKlineDataPackage " << e.what() << '\n';
    }    
    return package;
}

PackagePtr GetNewRspRiskCtrledDepthDataPackage(const SDepthData& depth, ID_TYPE socket_id, COMM_TYPE socket_type, int package_id)
{
    PackagePtr package =PackagePtr{new Package{}};

    try
    {        
        package->SetPackageID(package_id);

        package->prepare_response(UT_FID_RspRiskCtrledDepthData, package_id); 

        CREATE_FIELD(package, RspRiskCtrledDepthData);

        RspRiskCtrledDepthData* p_riskctrled_depth_data = GET_NON_CONST_FIELD(package, RspRiskCtrledDepthData);

        p_riskctrled_depth_data->set(&depth, socket_id, socket_type);               
    }
    catch(const std::exception& e)
    {
        std::cerr <<"GetNewRspRiskCtrledDepthDataPackage " << e.what() << '\n';
    }
    
    return package;    
}

PackagePtr GetNewRspSymbolListDataPackage(std::set<string> symbols, ID_TYPE socket_id, COMM_TYPE socket_type, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        package->prepare_response(UT_FID_RspSymbolListData, package_id);

        CREATE_FIELD(package, RspSymbolListData);

        RspSymbolListData* p_symbol_data = GET_NON_CONST_FIELD(package, RspSymbolListData);

        p_symbol_data->set(symbols, socket_id, socket_type);
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;       
}

PackagePtr GetReqEnquiryPackage(string symbol, double volume, double amount, int type, HttpResponseThreadSafePtr res)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        ID_TYPE id = ID_MANAGER->get_id();
        package->SetPackageID(id);
        package->prepare_request(UT_FID_ReqEnquiry, id);

        CREATE_FIELD(package, ReqEnquiry);

        ReqEnquiry* p_req_enquiry = GET_NON_CONST_FIELD(package, ReqEnquiry);

        // p_req_enquiry->set(symbol, volume, amount, type, res);        
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;  
}

PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, std::vector<KlineDataPtr>& kline_data_vec, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        CREATE_FIELD(package, RspKLineData);

        RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

        p_rsp_kline_data->set(pReqKlineData, kline_data_vec);

        // p_rsp_kline_data->set(pReqKlineData->symbol_, pReqKlineData->start_time_, pReqKlineData->end_time_,
        //                       pReqKlineData->frequency_, pReqKlineData->data_count_,
        //                       pReqKlineData->socket_id_, pReqKlineData->socket_type_, 
        //                       false, main_data);

        // assign(p_rsp_kline_data->socket_type, pReqKlineData->socket_type);
        // assign(p_rsp_kline_data->http_response_, pReqKlineData->http_response_);
        // assign(p_rsp_kline_data->websocket_, pReqKlineData->websocket_);

        // assign(p_rsp_kline_data->symbol_, pReqKlineData->symbol_);
        // assign(p_rsp_kline_data->start_time_, pReqKlineData->start_time_);
        // assign(p_rsp_kline_data->end_time_, pReqKlineData->end_time_);
        // assign(p_rsp_kline_data->frequency_, pReqKlineData->frequency_);
        // assign(p_rsp_kline_data->data_count_, main_data.size());

        // for (KlineDataPtr atom_kline:main_data)
        // {
        //     p_rsp_kline_data->kline_data_vec_.emplace_back(atom_kline);
        // }

        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;        
}


PackagePtr GetNewRspKLineDataPackage(ReqKLineData * pReqKlineData, KlineDataPtr& update_kline_data, int package_id)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        package->SetPackageID(package_id);

        CREATE_FIELD(package, RspKLineData);

        RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

        p_rsp_kline_data->set(pReqKlineData, update_kline_data);

        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;        
}

// PackagePtr GetNewRspKLineDataPackage(string symbol, type_tick start_time, type_tick end_time, int data_count,
//                                      frequency_type frequency, WebsocketClassThreadSafePtr ws,
//                                      KlineDataPtr& update_kline_data, int package_id)
// {
//     PackagePtr package = PackagePtr{new Package{}};
//     try
//     {    
//         package->SetPackageID(package_id);

//         CREATE_FIELD(package, RspKLineData);

//         RspKLineData* p_rsp_kline_data = GET_NON_CONST_FIELD(package, RspKLineData);

//         p_rsp_kline_data->is_update = true;

//         assign(p_rsp_kline_data->socket_type, COMM_TYPE::WEBSOCKET);
//         assign(p_rsp_kline_data->websocket_, ws);

//         assign(p_rsp_kline_data->symbol_, symbol);
//         assign(p_rsp_kline_data->start_time_, start_time);
//         assign(p_rsp_kline_data->end_time_, end_time);
//         assign(p_rsp_kline_data->frequency_, frequency);

//         p_rsp_kline_data->kline_data_vec_.emplace_back(update_kline_data);

//         return package;
//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
//     }
    
//     return package;      
// }                                     

PackagePtr GetRspEnquiryPackage(string symbol, double price, ID_TYPE socket_id, COMM_TYPE socket_type)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        ID_TYPE id = ID_MANAGER->get_id();
        package->SetPackageID(id);
        package->prepare_response(UT_FID_RspEnquiry, id);

        CREATE_FIELD(package, RspEnquiry);

        RspEnquiry* p_rsp_enquiry_data = GET_NON_CONST_FIELD(package, RspEnquiry);

        p_rsp_enquiry_data->set(symbol, price, socket_id, socket_type);
    
        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;      
}

PackagePtr GetRspErrMsgPackage(string err_msg, int err_id,  ID_TYPE socket_id, COMM_TYPE socket_type)
{
    PackagePtr package = PackagePtr{new Package{}};
    try
    {    
        ID_TYPE id = ID_MANAGER->get_id();
        package->SetPackageID(id);
        package->prepare_response(UT_FID_RspErrorMsg, id);

        CREATE_FIELD(package, RspErrorMsg);

        RspErrorMsg* p_rsp_err = GET_NON_CONST_FIELD(package, RspErrorMsg);

        p_rsp_err->set(err_msg, err_id, socket_id, socket_type);
    
        return package;
    }
    catch(const std::exception& e)
    {
        std::cerr << "GetNewRspSymbolListDataPackage: " << e.what() << '\n';
    }
    
    return package;
}                                