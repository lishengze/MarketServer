#include "server.h"
#include "rpc.h"
#include "../Log/log.h"

GrpcServer::~GrpcServer()
{
    if (cq_thread_.joinable())
    {
        cq_thread_.join();
    }
}

void GrpcServer::start()
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
        LOG_ERROR(e.what());
    }    
}

void GrpcServer::run_cq_loop()
{
    try
    {
        LOG_INFO("run_cq_loop");
        void* tag;
        bool status;
        while(true)
        {
            std::cout << "\n++++++++ Loop Start " << " ++++++++"<< std::endl;

            bool result = cq_->Next(&tag, &status);

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
                    rpc->process();
                }
                else
                {
                    reconnect_rpc(rpc);
                }                
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void GrpcServer::reconnect_rpc(BaseRPC* rpc)
{

}
