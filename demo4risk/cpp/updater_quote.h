#pragma once

#include "base/cpp/grpc_client.h"
#include "stream_engine.grpc.pb.h"
#include "risk_controller_config.h"
#include "updater_configuration.h"

using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using quote::service::v1::StreamEngine;
using quote::service::v1::SubscribeQuoteReq;
using quote::service::v1::SubscribeMixQuoteReq;
using SEMultiData = quote::service::v1::MultiMarketStreamDataWithDecimal;
using SEData = quote::service::v1::MarketStreamDataWithDecimal;
using SEDepth = quote::service::v1::DepthWithDecimal;
using SEDecimal = quote::service::v1::Decimal;
using quote::service::v1::DataInBinary;


class IQuoteUpdater {
public:
    virtual void on_snap(const SEData& quote) = 0;
};

class QuoteUpdater 
{
public:
    QuoteUpdater(){}
    ~QuoteUpdater(){}

    void start(const string& addr, IQuoteUpdater* callback) {
        thread_loop_ = new std::thread(&QuoteUpdater::_run, this, addr, callback);
    }

private:

    void _request(std::shared_ptr<grpc::Channel> channel, IQuoteUpdater* callback) {
        std::unique_ptr<StreamEngine::Stub> stub = StreamEngine::NewStub(channel);

        SubscribeQuoteReq req;
        req.set_exchange(MIX_EXCHANGE_NAME);
        //SubscribeMixQuoteReq req;
        SEMultiData multiQuote;
        DataInBinary dataInBinary;
        ClientContext context;

        //std::unique_ptr<ClientReader<SEMultiData> > reader(stub->SubscribeMixQuote(&context, req));
        std::unique_ptr<ClientReader<DataInBinary> > reader(stub->SubscribeQuoteInBinary(&context, req));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                _log_and_print("status is GRPC_CHANNEL_IDLE");
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                _log_and_print("status is GRPC_CHANNEL_CONNECTING");
                break;
            }
            case GRPC_CHANNEL_READY: {           
                _log_and_print("status is GRPC_CHANNEL_READY");
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                _log_and_print("status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                _log_and_print("status is GRPC_CHANNEL_SHUTDOWN");
                break;
            }
        }

        while (reader->Read(&dataInBinary)) {
            const string& data = dataInBinary.data();                
            if( !this->_decode_one_package(callback, data) )
                break;
        }
        /*
        while (reader->Read(&multiQuote)) {
            for( int i = 0 ; i < multiQuote.quotes_size() ; ++ i ) {
                const SEData& quote = multiQuote.quotes(i);
                callback->on_snap(quote);
            }
        }*/
        Status status = reader->Finish();
        if (status.ok()) {
            _log_and_print("MultiSubscribeQuote rpc succeeded.");
        } else {
            _log_and_print("MultiSubscribeQuote rpc failed.");
        }
    }

    bool _decode_one_package(IQuoteUpdater* callback, const string& data)
    {
        std::string::size_type startpos = 0;
        while( startpos < data.size() )
        {
            std::string::size_type pos = data.find(";", startpos);
            if( pos == std::string::npos )
                return false;
            std::string::size_type pos2 = data.find(";", pos+1);
            if( pos2 == std::string::npos )
                return false;
            
            // length
            uint64 length = ToUint64(data.substr(startpos, pos));

            // datatype
            uint64 datatype = ToUint64(data.substr(pos+1, pos2-pos-1));

            string quoteData = data.substr(pos2+1, length);

            if( datatype == QUOTE_TYPE_DEPTH ) {
                SEData quote;
                if( !quote.ParseFromString(quoteData) )
                    return false;

                // if (!RISK_CONFIG->check_symbol(quote.symbol()))
                // {
                //     continue;
                // }

                callback->on_snap(quote);
            } else if( datatype == QUOTE_TYPE_TRADE ) {
            }
            startpos = pos2 + 1 + length;
            //cout << startpos << ", " << data.size() << endl;
        } 
        return true;
    }

    void _run(const string& addr, IQuoteUpdater* callback) 
    {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

        while( 1 ) {            
            _request(channel, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};