#include "grpc_server.h"

void ServerImpl::register_client(CallDataServeMarketStream* calldata)
{
    dc_->add_client(calldata);
}

void ServerImpl::unregister_client(CallDataServeMarketStream* calldata)
{
    dc_->del_client(calldata);
}
