#include "rpc_server.h"
#include "rpc.h"
#include "comm_interface_define.h"
#include "comm_declare.h"

COMM_NAMESPACE_START

GrpcServer::~GrpcServer()
{
    if (cq_thread_.joinable())
    {
        COMM_LOG_INFO("cq_thread_ join");
        cq_thread_.join();
    }
}

void GrpcServer::start()
{
    try
    {
        init_async_server_env();

        init_rpc();

        init_cq_thread();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void GrpcServer::init_async_server_env()
{
    try
    {
        COMM_LOG_INFO("BaseServer listen: " + address_);

        builder_.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder_.RegisterService(&service_);

        cq_ = builder_.AddCompletionQueue();
        server_ = builder_.BuildAndStart();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

void GrpcServer::init_rpc()
{
    try
    {
        COMM_LOG_INFO("Init request_trade_data_rpc_");

        request_trade_data_rpc_ = new RequestTradeDataRPC(cq_.get(), &service_);
        request_trade_data_rpc_->register_server(this);
        request_trade_data_rpc_->start();

        // get_stream_trade_data_rpc = new GetTradeStreamDataRPC(cq_.get(), &service_);
        // get_stream_trade_data_rpc->register_server(this);
        // get_stream_trade_data_rpc->start();

    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

void GrpcServer::init_cq_thread()
{
    try
    {
        cq_thread_ = std::thread(&GrpcServer::run_cq_loop, this);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

void GrpcServer::run_cq_loop()
{
    try
    {
        COMM_LOG_INFO("run_cq_loop");
        void* tag;
        bool status;
        while(true)
        {
            // std::cout << "\n++++++++ Loop Start " << " ++++++++"<< std::endl;
            
            bool result = cq_->Next(&tag, &status);

            COMM_LOG_INFO("----- CQ WakeUp ----- " 
                    + ", result: " + std::to_string(result) 
                    + ", status: " + std::to_string(status));

            // cout << "cq loop server_spi class: " << server_spi_->get_class_name() 
            //      << " thread_id: " << std::this_thread::get_id() 
            //      << " vp: " << *(int *)server_spi_ 
            //      << endl;            

            BaseRPC* rpc = static_cast<BaseRPC*>(tag);

            // if (dead_rpc_set_.find(rpc) != dead_rpc_set_.end())
            // {
            //     cout << "[E] RPC id=" << rpc->obj_id_ << " has been released!" << endl;

            //     std::cout << "[E][CQ] result: "<<  result << " status: " << status  
            //               << ", session_id_=" << rpc->session_id_ 
            //               << ", rpc_id_=" << rpc->rpc_id_ 
            //               << ", obj_id: " << rpc->obj_id_ 
            //               << std::endl;

            //     continue;
            // }

            if (rpc->is_stream_)
            {

                // check_dead_rpc(rpc);

                // if (result && status)
                // {                
                //     rpc->process();
                // }
                // else
                // {
                //     record_dead_rpc(rpc);

                //     reconnect(rpc);
                // }
            }
            else
            {
                
                if (result && status)
                {                
                    if (rpc->is_inner_cq_event_)
                    {
                        rpc->is_inner_cq_event_ = false;

                        COMM_LOG_INFO(rpc->get_rpc_name() + " is inner write!");
                    }
                    else if (rpc->is_finished_)
                    {
                        COMM_LOG_INFO(rpc->get_rpc_name() + " is finished! *********** \n");



                        // reconnect_rpc(rpc);
                    }
                    else
                    {
                        rpc->process();
                    }
                    
                }
                else
                {
                    COMM_LOG_INFO(rpc->get_rpc_name() + " is over");

                    reconnect_rpc(rpc);
                }                
            }
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void GrpcServer::reconnect_rpc(BaseRPC* rpc)
{
    try
    {
        rpc->create_rpc_for_next_client();

        delete rpc;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

bool GrpcServer::get_req_trade_info(const ReqTradeData& req_trade, TradeData& dst_trade_data)
{
    try
    {
        return server_engine_->get_req_trade_info(req_trade, dst_trade_data);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

COMM_NAMESPACE_END