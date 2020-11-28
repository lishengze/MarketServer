#include "rest_server.h"

RestServer::RestServer()
{
    rest_server_ = boost::make_shared<uWS::App>();
}

RestServer::~RestServer()
{
    if (!listen_thread_ && listen_thread_->joinable())
    {
        listen_thread_->join();
    }
}

void RestServer::launch()
{
    listen_thread_ = boost::make_shared<std::thread>(&RestServer::listen, this);
}

void RestServer::listen()
{
    rest_server_->listen(server_port_, [this](auto* token){
        if (token) {
            std::cout << "Listening on port " << server_port_ << std::endl;
        }
    }).get("/*", [this](uWS::HttpResponse<false> * response, uWS::HttpRequest * request){
        process_get(response, request);
    }).post("/*", [this](uWS::HttpResponse<false> * response, uWS::HttpRequest * request){
        process_post(response, request);
    }).del("/*", [this](uWS::HttpResponse<false> * response, uWS::HttpRequest * request){
        process_del(response, request);
    }).put("/*", [this](uWS::HttpResponse<false> * response, uWS::HttpRequest * request){
        process_put(response, request);
    }).run();
}

void RestServer::release()
{

}

void RestServer::process_get(uWS::HttpResponse<false> * response, uWS::HttpRequest * request)
{

}

void RestServer::process_post(uWS::HttpResponse<false> * response, uWS::HttpRequest *request)
{

}

void RestServer::process_del(uWS::HttpResponse<false> * response, uWS::HttpRequest *request)
{

}

void RestServer::process_put(uWS::HttpResponse<false> * response, uWS::HttpRequest *request)
{

}