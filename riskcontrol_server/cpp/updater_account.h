#pragma once

#include "base/cpp/grpc_client.h"
#include "data_struct/data_struct.h"
#include "base/cpp/basic.h"
#include "Log/log.h"

#include "risk_interface_define.h"

using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using asset::service::v1::Asset;
using asset::service::v1::AccountStreamData;
using asset::service::v1::AccountData;


class AccountUpdater {
public:
    AccountUpdater(){}
    ~AccountUpdater(){}

    void start(const string& addr, IAccountUpdater* callback) {
        thread_loop_ = new std::thread(&AccountUpdater::_run, this, addr, callback);
    }

private:
    void _request(std::shared_ptr<grpc::Channel> channel, IAccountUpdater* callback) 
    {
        std::unique_ptr<Asset::Stub> stub = Asset::NewStub(channel);

        google::protobuf::Empty req;
        AccountStreamData multiAccount;
        ClientContext context;

        std::unique_ptr<ClientReader<AccountStreamData> > reader(stub->GetAccountStream(&context, req));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                LOG_INFO("AccountUpdater: status is GRPC_CHANNEL_IDLE");
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                LOG_INFO("AccountUpdater: status is GRPC_CHANNEL_CONNECTING");
                break;
            }
            case GRPC_CHANNEL_READY: {           
                LOG_INFO("AccountUpdater: status is GRPC_CHANNEL_READY");
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                LOG_WARN("AccountUpdater: status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                LOG_INFO("AccountUpdater: status is GRPC_CHANNEL_SHUTDOWN");
                break;
            }
        }

        LOG_DEBUG("\n AccountUpdater Begin Read Data  ");

        while (reader->Read(&multiAccount)) {
            {
                LOG_DEBUG("\n AccountUpdater Read Data: account_data.size: " + std::to_string(multiAccount.account_data_size()));

                std::unique_lock<std::mutex> inner_lock{ mutex_account_ };
                for( int i = 0 ; i < multiAccount.account_data_size() ; ++ i ) {
                    const AccountData& account = multiAccount.account_data(i);
                    LOG_INFO("update accout: "+  account.exchange_id() + "." + account.currency() + " " +   std::to_string(account.available()));
                    LOG_DEBUG("update accout: "+  account.exchange_id() + "." + account.currency() + " " +   std::to_string(account.available()));
                    account_.hedge_accounts_[account.exchange_id()].currencies[account.currency()].amount = account.available();
                }
            }            

            callback->on_account_update(account_);
        }

        Status status = reader->Finish();
        if (status.ok()) {
            LOG_INFO("AccountUpdater rpc succeeded.");
        } else {
            LOG_WARN("AccountUpdater rpc failed.");
        }
    }

    void _run(const string& addr, IAccountUpdater* callback) 
    {
        /*
        std::this_thread::sleep_for(std::chrono::seconds(10));
        for( int i = 0 ; i < 1000000 ; i ++ ){
            account_.hedge_accounts_["ART"].currencies["ABC"].amount = 99999999;
            account_.hedge_accounts_["ART"].currencies["USDT"].amount = 99999999;
            account_.hedge_accounts_["ALAMEDA"].currencies["BTC"].amount = 99999999;
            account_.hedge_accounts_["ALAMEDA"].currencies["USDT"].amount = 99999999;
            account_.hedge_accounts_["HUOBI"].currencies["BTC"].amount = 99999999;
            account_.hedge_accounts_["HUOBI"].currencies["USDT"].amount = 99999999 - i;
            account_.hedge_accounts_["BINANCE"].currencies["BTC"].amount = 99999999;
            account_.hedge_accounts_["BINANCE"].currencies["USDT"].amount = 99999999;
            callback->on_account_update(account_);
        }*/        
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

        while( 1 ) {            
            _request(channel, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;

    mutable std::mutex         mutex_account_;
    AccountInfo                account_;
};