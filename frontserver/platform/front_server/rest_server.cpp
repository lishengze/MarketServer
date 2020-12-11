#include <boost/algorithm/string.hpp>

#include "rest_server.h"
#include "front_server.h"
#include "../config/config.h"
#include "../front_server_declare.h"
#include "../util/tools.h"
#include "../log/log.h"

string RestServer::VERSION1 = "v1";
string RestServer::VERSION2 = "v2";
string RestServer::KLINE_REQUEST = "kline_request";
string RestServer::KLINE_REQUEST_SYMBOL = "symbol";
string RestServer::KLINE_REQUEST_STARTTIME = "start_time";
string RestServer::KLINE_REQUEST_ENDTIME = "end_time";
string RestServer::KLINE_REQUEST_FREQUENCY = "frequency";

RestServer::RestServer(utrade::pandora::io_service_pool& pool):ThreadBasePool(pool)
{
    server_port_ = CONFIG->get_rest_port();
}

RestServer::~RestServer()
{
    if (!listen_thread_ && listen_thread_->joinable())
    {
        listen_thread_->join();
    }
}

void httpResponseOnAborted()
{
    cout << "\n****** httpResponseOnAborted *****" << endl;
}


void RestServer::launch()
{
    listen_thread_ = boost::make_shared<std::thread>(&RestServer::listen, this);
}

string get_rsp_msg()
{
    string response_start_line = "HTTP/1.1 200 OK\r\n";
    string response_headers = "Server: My server\r\n";
    string response_body = "<h1>Python HTTP Test</h1>";
    string response = response_start_line + response_headers + "\r\n" + response_body;
    return response;
}

void RestServer::listen()
{
    uWS::App().get("/*", [this](HttpResponse * response, HttpRequest * request) {
        cout << "*** Get ***" << endl;

        response->onAborted(httpResponseOnAborted);

        cout << "AbortEnd!" << endl;

        // get_io_service().post(std::bind(&RestServer::process_get, this, response, request));

        process_get(response, request);

    }).post("/*", [this](HttpResponse * response, HttpRequest * request){
        process_post(response, request);
    }).del("/*", [this](HttpResponse * response, HttpRequest * request){
        process_del(response, request);
    }).put("/*", [this](HttpResponse * response, HttpRequest * request){
        process_put(response, request);
    }).listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "Rest Listening on port " << server_port_ << std::endl;
        }
    }).run();


    // uWS::App().get("/*", &RestServer::process_get){
    //     process_get(response, request);
    // }).post("/*", [this](HttpResponse * response, HttpRequest * request){
    //     process_post(response, request);
    // }).del("/*", [this](HttpResponse * response, HttpRequest * request){
    //     process_del(response, request);
    // }).put("/*", [this](HttpResponse * response, HttpRequest * request){
    //     process_put(response, request);
    // }).listen(server_port_, [this](auto* token){
    //     if (token) {
    //         std::cout << "Rest Listening on port " << server_port_ << std::endl;
    //     }
    // }).run();

     cout << "RestServer Listen End!" << endl;
}

void RestServer::release()
{
    
}

void RestServer::test_response_multithread(HttpResponse * rsp)
{
    std::thread test_thread = std::thread(&RestServer::test_response_multithread_run, this, rsp);

    // test_response_multithread_run(rsp);
}

void RestServer::test_response_multithread_run(HttpResponse * rsp)
{
    cout << "RestServer::test_response_multithread_run" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    rsp->end(get_rsp_msg());
}

void RestServer::process_get(HttpResponse* response, HttpRequest* request)
{
    cout << "RestServer::process_get " << endl;

    try
    {
        // response->end(get_rsp_msg());
        // response->tryEnd(get_rsp_msg());
        // test_response_multithread(response);
        // return;

        cout << "getUrl: " << request->getUrl() << "; \n"
            << "getMethod: " << request->getMethod() << "; \n"
            << "getQuery: " << request->getQuery() << "; \n"
            << "getParameter: " << request->getParameter(1) << endl;

        string url = string(request->getUrl().data(), request->getUrl().size());

        cout << "url: " << url << endl;

        string first_split_char = "/";
        std::vector<std::string> first_vec;
        boost::split(first_vec, url, boost::is_any_of(first_split_char));

        string error_msg="";

        cout << "first_vec.size: " << first_vec.size() << endl;
        for (string data:first_vec)
        {
            cout << data << " \n";
        }
        cout << endl;

        if (first_vec.size() != 3)
        {
            error_msg = "Url can only has tow / ";
            send_err_msg(response, error_msg);
            return;
        }
        else
        {
            if (first_vec[0] == RestServer::VERSION1)
            {
                if (first_vec[1] == RestServer::KLINE_REQUEST)
                {
                    if (!process_v1_request_kline(first_vec[2], error_msg, response, request))             
                    {
                        send_err_msg(response, error_msg);
                        return;
                    }
                }
            }
        }
        return;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

bool RestServer::process_v1_request_kline(string& query_param, string& error_msg, HttpResponse * res, HttpRequest * req)
{
    cout << "RestServer::process_v1_request_kline " << endl;
    bool result = false;
    if (query_param.find(RestServer::KLINE_REQUEST_SYMBOL) == std::string::npos)
    {
        error_msg += "Query Parmas Lost Param: " + RestServer::KLINE_REQUEST_SYMBOL;
    }    
    else if (query_param.find(RestServer::KLINE_REQUEST_STARTTIME) == std::string::npos)
    {
        error_msg += "Query Parmas Lost Param: " + RestServer::KLINE_REQUEST_STARTTIME;
    }
    else if (query_param.find(RestServer::KLINE_REQUEST_ENDTIME) == std::string::npos)
    {
        error_msg += "Query Parmas Lost Param: " + RestServer::KLINE_REQUEST_ENDTIME;
    }
    else if (query_param.find(RestServer::KLINE_REQUEST_FREQUENCY) == std::string::npos)
    {
        error_msg += "Query Parmas Lost Param: " + RestServer::KLINE_REQUEST_FREQUENCY;
    }
    else
    {
        string query_split_char = "&";
        std::vector<std::string> param_vec;
        boost::split(param_vec, query_param, boost::is_any_of(query_split_char));      

        for (string param:param_vec)
        {
            cout << param << "\n";
        }          
        cout << endl;

        string symbol = param_vec[0].substr(param_vec[0].find("=")+1);
        type_tick start_time = trans_string_to_type_tick(param_vec[1].substr(param_vec[1].find("=")+1));
        type_tick end_time = trans_string_to_type_tick(param_vec[2].substr(param_vec[2].find("=")+1));
        type_tick frequency = trans_string_to_type_tick(param_vec[3].substr(param_vec[3].find("=")+1));

        cout<< "symbol: " << symbol << " \n" 
            << "start_time: " << start_time << " \n"
            << "end_time: " << end_time << " \n"
            << "frequency: " << frequency << " \n"
            << endl;

        if (start_time % (60) != 0)
        {
            error_msg += "start_time is error: " + std::to_string(start_time);
        }
        else if (end_time %(60) != 0)
        {
            error_msg += "end_time is error: " + std::to_string(end_time);
        }
        else
        {
            if (!front_server_->request_kline_data(symbol, start_time, end_time, frequency, res, req))
            {
                error_msg += "Server Internel Error, Please Try Again!";
            }
            else
            {
                result = true;
            }            
        }   
    }       
    return result;
}

void RestServer::process_post(HttpResponse * response, HttpRequest *request)
{

}

void RestServer::process_del(HttpResponse * response, HttpRequest *request)
{

}

void RestServer::process_put(HttpResponse * response, HttpRequest *request)
{

}

void RestServer::set_front_server(FrontServer* front_server)
{
    front_server_ = front_server;
}

void RestServer::send_err_msg(HttpResponse * response, string msg)
{
    try
    {
        LOG_INFO(string("send err msg: ") + msg);
        nlohmann::json json_data;     
        json_data["err_msg"] = msg;
        response->end(json_data.dump());
    }
    catch(const std::exception& e)
    {
        std::stringstream stream_obj;
        stream_obj << "[E]  RestServer::send_err_m...sg: " << e.what() << "\n";
        LOG_ERROR(stream_obj.str());
    }
}