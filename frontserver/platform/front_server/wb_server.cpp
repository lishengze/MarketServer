#include <boost/shared_ptr.hpp>
#include <functional>
#include <sstream>

#include "pandora/util/json.hpp"
#include "pandora/util/time_util.h"

#include "wb_server.h"
#include "front_server.h"

#include "../util/tools.h"
#include "../util/package_manage.h"
#include "../ErrorDefine.hpp"
#include "../util/id.hpp"
#include "../config/config.h"
#include "../log/log.h"

#include "../front_server_declare.h"


using namespace std::placeholders;

WBServer::WBServer()
{
    server_port_ = CONFIG->get_ws_port();

    init_websocket_options();

    init_websocket_behavior();

    init_websocket_ssl_server();

    init_websocket_server();

    start_heartbeat();
}

WBServer::~WBServer()
{
    if (!listen_thread_ && listen_thread_->joinable())
    {
        listen_thread_->join();
    }

    if (!heartbeat_thread_ && heartbeat_thread_->joinable())
    {
        heartbeat_thread_->join();
    }    
}

void WBServer::init_websocket_options()
{

}

void WBServer::init_websocket_behavior()
{
    websocket_behavior_.open = std::bind(&WBServer::on_open, this, std::placeholders::_1);
    websocket_behavior_.message = std::bind(&WBServer::on_message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    websocket_behavior_.ping = std::bind(&WBServer::on_ping, this, std::placeholders::_1);
    websocket_behavior_.pong = std::bind(&WBServer::on_pong, this, std::placeholders::_1);
    websocket_behavior_.close = std::bind(&WBServer::on_close, this, std::placeholders::_1);
}

void WBServer::init_websocket_ssl_server()
{
}

void WBServer::init_websocket_server()
{
    // wss_server_(std::move(uWS::App().ws<PerSocketData>("/*", std::move(websocket_behavior_))));
}

void WBServer::set_front_server(FrontServer* front_server)
{
    front_server_ = front_server;
}

void WBServer::on_open(WebsocketClass * ws)
{
    process_on_open(ws);
}

// 处理各种请求
void WBServer::on_message(WebsocketClass * ws, std::string_view msg, uWS::OpCode code)
{
    try
    {
    
        string trans_msg(msg.data(), msg.size());

        process_on_message(trans_msg, ws);
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_message: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_message: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }

}

void WBServer::on_ping(WebsocketClass * ws)
{

}

void WBServer::on_pong(WebsocketClass * ws)
{

}

void WBServer::on_close(WebsocketClass * ws)
{
    try
    {
        cout << "one connection closed " << endl;

        clean_ws(ws);
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_close: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::on_close: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void WBServer::listen()
{
    try
    {
        cout << "WServer Start Listen: " << server_port_ << endl;

        uWS::App().ws<WSData>("/*", std::move(websocket_behavior_)).listen(server_port_, [this](auto* token){
            if (token) {
                std::cout << "WS Listening on port " << server_port_ << std::endl;
            }
        }).run();

        cout << "WServer Listen End!" << endl;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::listen(): " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::listen(): unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
    

}

void WBServer::launch()
{
    listen_thread_ = std::make_shared<std::thread>(&WBServer::listen, this);
}

void WBServer::start_heartbeat()
{
    try
    {
        heartbeat_thread_ = std::make_shared<std::thread>(&WBServer::heartbeat_run, this);
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::start_heartbeat: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::start_heartbeat: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void WBServer::release()
{

}

void WBServer::process_on_open(WebsocketClass * ws)
{
    try
    {
        cout << "\nWBServer::on_open ws: " << ws << endl;

        ID_TYPE socket_id = store_ws(ws);

        LOG->record_client_info(std::to_string(socket_id), " on_open");

        PackagePtr package =  GetReqSymbolListDataPackage(socket_id, COMM_TYPE::WEBSOCKET, ID_MANAGER->get_id());

        if (package)
        {
            cout << "WBServer::process_on_open Request SymbolList" << endl;
            front_server_->deliver_request(package);
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_on_opene: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_on_open: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }

}

void WBServer::process_on_message(string ori_msg, WebsocketClass * ws)
{
    try
    {
        // cout << utrade::pandora::NanoTimeStr() << " ws: " << ws << " " << ori_msg << endl;

        ID_TYPE socket_id = check_ws(ws);
        if (socket_id)
        {
            LOG->record_client_info(std::to_string(socket_id), ori_msg);

            nlohmann::json js = nlohmann::json::parse(ori_msg);

            if (js["type"].is_null())
            {
                ws->send(get_error_send_rsp_string("Lost type Item!"), uWS::OpCode::TEXT);
            }
            else
            {
                if (js["type"].get<string>() == "sub_symbol")
                {
                    process_depth_req(ori_msg, socket_id);

                    process_trade_req(ori_msg, socket_id);
                }

                if (js["type"].get<string>() == HEARTBEAT)
                {
                    process_heartbeat(socket_id);
                }

                if (js["type"].get<string>() == KLINE_UPDATE)
                {
                    process_kline_req(ori_msg, socket_id);
                }     

                if (js["type"].get<string>() == "trade")
                {
                    process_trade_req(ori_msg, socket_id);
                }   

                // cout << "WBServer::process_on_message Type: " << js["type"].get<string>() << endl;
            }
        }
        else
        {
            LOG->record_client_info(std::to_string(socket_id), " id is invalid");

            cout << "WBServer::process_on_message ws: " << ws << ", socket_id: " << socket_id << " is invalid" << endl;
        }
 
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_on_message: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }      
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_on_message: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void WBServer::process_kline_req(string ori_msg, ID_TYPE socket_id)
{
    // return;  

    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        int error_id = 0;
        string err_msg ="";
        string symbol;
        type_tick start_time = 0;
        type_tick end_time = 0;
        type_tick frequency;
        int data_count = -1;

        if (js["symbol"].is_null())
        {
            err_msg += "Lost Symbol Item \n";
            error_id = -1;
        }
        else
        {
            symbol = js["symbol"].get<string>();
        }
        

        if (!js["start_time"].is_null() && !js["end_time"].is_null())
        {
            start_time = std::stoul(js["start_time"].get<string>());
            end_time = std::stoul(js["end_time"].get<string>());
        }
        else if (!js["data_count"].is_null())
        {
            data_count = std::stoul(js["data_count"].get<string>());
        }
        else
        {
            err_msg += "K line data needs start_end time or data_count! \n";
            error_id = -1;
        }

        if (js["frequency"].is_null())
        {
            err_msg += "Lost frequency Item \n";
            error_id = -1;
        }        
        else
        {
            frequency = std::stoi(js["frequency"].get<string>());
        }

        stringstream info_obj;
        info_obj<< "symbol: " << symbol << " \n" 
                << "start_time: " << start_time << " \n"
                << "end_time: " << end_time << " \n"
                << "frequency: " << frequency << " \n"
                << "data_count: " << data_count << "\n";

        LOG_INFO(info_obj.str());

        { 
            if (error_id == 0)
            {
                COMM_TYPE socket_type = COMM_TYPE::WEBSOCKET;
                PackagePtr package = CreatePackage<ReqKLineData>(symbol, start_time, end_time, data_count, frequency, 
                                                                 socket_id, socket_type);
                
                if (package)
                {
                    package->prepare_request(UT_FID_ReqKLineData, package->PackageID());
                    front_server_->deliver_request(package);
                }
                else
                {
                    LOG_ERROR("WBServer::process_kline_req CreatePackage Failed!");
                }

                // PackagePtr package = PackagePtr{new Package{}};
                // package->SetPackageID(ID_MANAGER->get_id());
                // CREATE_FIELD(package, ReqKLineData);
                // ReqKLineData* p_req_kline_data = GET_NON_CONST_FIELD(package, ReqKLineData);

                // if (p_req_kline_data)
                // {
                //     p_req_kline_data->set(symbol, start_time, end_time, data_count, frequency, 
                //                           socket_id, COMM_TYPE::WEBSOCKET);  
                //     front_server_->deliver_request(package);
                // }                   
            }
            else
            {
                LOG_ERROR(err_msg);

                if (wss_con_map_.find(socket_id) != wss_con_map_.end())
                {
                    wss_con_map_[socket_id]->send(get_error_send_rsp_string(err_msg));
                }
                else
                {
                    stringstream s_obj;
                    s_obj << "WBServer::process_kline_req websocket socket_id " << socket_id << " is invalid \n";
                    LOG_ERROR(s_obj.str());
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] WBServer::process_kline_data " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_kline_data: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void WBServer::process_depth_req(string ori_msg, ID_TYPE socket_id)
{
    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];

            for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
            {
                string cur_symbol = *it;
                    
                PackagePtr request_depth_package = GetReqRiskCtrledDepthDataPackage(cur_symbol, socket_id, ID_MANAGER->get_id());

                if (request_depth_package)
                {
                    front_server_->deliver_request(request_depth_package);
                }
                else
                {
                    LOG_ERROR("WBServer::process_depth_req GetReqRiskCtrledDepthDataPackage Failed");
                }                
            }     
        }
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] WBServer::process_depth_req " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_depth_req: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void WBServer::process_trade_req(string ori_msg, ID_TYPE socket_id)
{
    try
    {
        cout << "ori_msg: " << ori_msg << endl;
        nlohmann::json js = nlohmann::json::parse(ori_msg);
        if (!js["symbol"].is_null())
        {
            nlohmann::json symbol_list = js["symbol"];
            for (json::iterator it = symbol_list.begin(); it != symbol_list.end(); ++it)
            {
                string symbol = *it;
                    
                PackagePtr package = CreatePackage<ReqTrade>(symbol, false, socket_id,  COMM_TYPE::WEBSOCKET);

                if (package)
                {
                    package->prepare_request(UT_FID_ReqTrade, ID_MANAGER->get_id());
                    front_server_->deliver_request(package);
                }
                else
                {
                    stringstream stream_msg;
                    stream_msg << "WBServer::process_trade_req Create Package Failed " << symbol << " " << socket_id << "\n";                
                    LOG_ERROR(stream_msg.str());
                }          
            }     
        }        
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] WBServer::process_trade_req " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::process_trade_req: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }  
    
}

void WBServer::process_heartbeat(ID_TYPE socket_id)
{
    try
    {
        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            wss_con_map_[socket_id]->set_alive(true);
            cout << "[S] "  << utrade::pandora::NanoTimeStr() << " "<< socket_id << " is alive" << endl;
        }        
        else
        {
            cout << "[E] process_heartbeat socket_id: " << socket_id << " is invalid " << endl;
        }

        // cout << "\nWBServer::process_heartbeat: " << socket_id << " " << wss_con_map_[socket_id] << endl;
    }
    catch(const std::exception& e)
    {
        stringstream stream_msg;
        stream_msg << "[E] WBServer::process_heartbeat " << e.what() << "\n";
        LOG_ERROR(stream_msg.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] FrontServer::response_depth_data_package: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

bool WBServer::send_data(ID_TYPE socket_id, string msg)
{
    try
    {
        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) != wss_con_map_.end())
        {
            wss_con_map_[socket_id]->send(msg);
            return true;
        }
        else
        {
            stringstream s_obj;
            s_obj << "WBServer::send_data websocket socket_id " << socket_id << " is invalid \n";
            LOG_ERROR(s_obj.str());
            return false;
        }    
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::send_data: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::send_data: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }    
}

void WBServer::heartbeat_run()
{
    try
    {
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(heartbeat_seconds_));

            check_heartbeat();
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::heartbeat_run: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::heartbeat_run: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

void WBServer::check_heartbeat()
{
    try
    {
        {
            std::vector<ID_TYPE> ws_vec;    

            std::lock_guard<std::mutex> lk(wss_con_mutex_);
            for (auto iter:wss_con_map_)
            {
                if (!iter.second->is_alive())
                {
                    ws_vec.push_back(iter.first);
                    cout <<"\n[H] " << utrade::pandora::NanoTimeStr() << " id: " << iter.first << " check heartbeat failed!" << endl;
                }
                else
                {
                    cout <<"\n[H] " << utrade::pandora::NanoTimeStr() << " id: " << iter.first << " check heartbeat Successfully!" << endl;                    
                }
            }
                
            for (auto socket_id:ws_vec)
            {
                // cout << utrade::pandora::NanoTimeStr() << " ws: " << wss_con_map_[socket_id] << " , id: " << socket_id << " check heartbeat failed!" << endl;
                WebsocketClass * ws = wss_con_map_[socket_id]->get_ws();
                WSData* data_ptr = (WSData*)(ws->getUserData());
                data_ptr->set_id(0);
                ws->close();

                wss_con_map_[socket_id]->set_alive(false);
                wss_con_map_.erase(socket_id);            
            }     
        }   

        string heartbeat_str = get_heartbeat_str();    
        for (auto iter:wss_con_map_)
        {
            iter.second->set_alive(false);
            iter.second->send(heartbeat_str);
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_heartbeat: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_heartbeat: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }

}

ID_TYPE WBServer::store_ws(WebsocketClass * ws)
{
    try
    {                
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();

        if (!socket_id)
        {
            socket_id = ID_MANAGER->get_id();
            data_ptr->set_id(socket_id);
        }

        std::lock_guard<std::mutex> lk(wss_con_mutex_);
        if (wss_con_map_.find(socket_id) == wss_con_map_.end())
        {
            WebsocketClassThreadSafePtr ws_safe = boost::make_shared<WebsocketClassThreadSafe>(ws, socket_id);
            ws_safe->set_alive(true);
            wss_con_map_[socket_id] = ws_safe;
            cout << "store ws: " << ws << " id: " << socket_id << endl;            
        }
        return socket_id;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::store_ws: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::store_ws: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

ID_TYPE WBServer::check_ws(WebsocketClass * ws)
{
    try
    {
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();
        if (socket_id)
        {
            if (wss_con_map_.find(socket_id) == wss_con_map_.end())
            {
                stringstream s_obj;
                s_obj << "ws " << ws << " id: " << socket_id << " was not stored in wss_con_map_!" << "\n";
                LOG_INFO(s_obj.str());

                data_ptr->set_id(0);
            }
            else
            {
                wss_con_map_[socket_id]->set_alive(true);
            }

            
            // cout << "\nWBServer::check_ws: " << data_ptr->get_id() << " " << ws << endl;
        }
        else
        {
            cout << "\nWBServer ws " << ws << " is invalid now! " << endl;
        }

        return data_ptr->get_id();
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_ws " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::check_ws: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }
}

ID_TYPE WBServer::clean_ws(WebsocketClass* ws)
{
    try
    {
        WSData* data_ptr = (WSData*)(ws->getUserData());
        ID_TYPE socket_id = data_ptr->get_id();

        if (socket_id)
        {
            data_ptr->set_id(0);

            std::lock_guard<std::mutex> lk(wss_con_mutex_);
            if (wss_con_map_.find(socket_id) != wss_con_map_.end())
            {
                wss_con_map_[socket_id]->set_alive(false);
                wss_con_map_.erase(socket_id);
                LOG->record_client_info(std::to_string(socket_id), " id cleaned successfully!");
                cout << "clean_ws Socket: " << ws << " id: " << socket_id << " successfully! " << endl;
            }
            else
            {
                LOG->record_client_info(std::to_string(socket_id), " id Already Cleaned!");
                cout << "clean_ws Socket: " << ws << " id: " << socket_id << " Already Cleaned! " << endl;
            }
            return socket_id;
        
        }
        else
        {
            LOG->record_client_info(std::to_string(socket_id), " id was not stored!");
            cout << "clean_ws Socket: " << socket_id << " was not stored!" << endl;
        }

        return 0;
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::clean_ws: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }    
    catch(...)
    {
        std::stringstream stream_obj;
        stream_obj << "[E] WBServer::clean_ws: unkonwn exception! " << "\n";
        LOG_ERROR(stream_obj.str());
    }

}

ID_TYPE WBServer::get_socket_id(WebsocketClass * ws)
{
    WSData* data_ptr = (WSData*)(ws->getUserData());
    return data_ptr->get_id();
}