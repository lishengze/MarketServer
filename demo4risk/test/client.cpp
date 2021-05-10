#include "client.h"

void SyncClient::PutOrderStream()
{
    try
    {
        cout << "SyncClient::PutOrderStream " << endl;
        ClientContext context;
        google::protobuf::Empty empty;

        TradedOrderStreamData request_data;
        request_data.set_direction(quote::service::v1::TradedOrderStreamData_Direction::TradedOrderStreamData_Direction_BUY);

        request_data.set_symbol("BTC_USDT");
        request_data.set_price("1.1");
        request_data.set_order_amount(1);
        request_data.set_traded(false);

        if (stub_)
        {
            std::unique_ptr< grpc::ClientWriter< ::quote::service::v1::TradedOrderStreamData>> client_writer = stub_->PutTradedOrderStream(&context, &empty);
            if (client_writer->Write(request_data))
            {
                if (client_writer->WritesDone())
                {

                }
                else
                {
                    cout << "client_writer->WritesDone() Failed! " << endl;
                }
            }
            else
            {
                cout << "SyncClient::PutOrderStream Write Failed! " << endl;
            }            
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"\n[E] SyncClient::PutOrderStream " << e.what() << '\n';
    }
    
}