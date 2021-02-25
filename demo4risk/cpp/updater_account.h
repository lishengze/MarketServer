#pragma once

#include "base/cpp/grpc_client.h"
#include "account.grpc.pb.h"
#include "risk_controller_config.h"

using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using asset::service::v1::Asset;
using asset::service::v1::AccountStreamData;
using asset::service::v1::AccountData;

struct CurrencyInfo {
    double amount;
};

struct HedgeAccountInfo
{
    unordered_map<TSymbol, CurrencyInfo> currencies;

    HedgeAccountInfo() {
    }
};

struct UserAccountInfo
{
    unordered_map<TSymbol, CurrencyInfo> currencies;

    UserAccountInfo() {
    }
};

struct AccountInfo
{
    unordered_map<TExchange, HedgeAccountInfo> hedge_accounts_;
    UserAccountInfo user_account_;

    double get_user_amount(const string& currency) const {
        return 0;
    }
    
    double get_hedge_amount(const string& currency) const {
        double total = 0;
        for( auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) {
            const HedgeAccountInfo& hedge = iter->second;
            auto iter2 = hedge.currencies.find(currency);
            if( iter2 != hedge.currencies.end() ) {
                total += iter2->second.amount;
            }
        }
        return total;
    }

    void get_hedge_amounts(const string& currency, double percent, unordered_map<TExchange, double>& amounts) const {
        for( auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) {
            const HedgeAccountInfo& hedge = iter->second;
            auto iter2 = hedge.currencies.find(currency);
            if( iter2 != hedge.currencies.end() ) {
                amounts[iter->first] = iter2->second.amount * percent / 100;
            }
        }
    }
};

class IAccountUpdater {
public:
    virtual void on_account_update(const AccountInfo& info) = 0;
};

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
                _log_and_print("AccountUpdater: status is GRPC_CHANNEL_IDLE");
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                _log_and_print("AccountUpdater: status is GRPC_CHANNEL_CONNECTING");
                break;
            }
            case GRPC_CHANNEL_READY: {           
                _log_and_print("AccountUpdater: status is GRPC_CHANNEL_READY");
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                _log_and_print("AccountUpdater: status is GRPC_CHANNEL_TRANSIENT_FAILURE");
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                _log_and_print("AccountUpdater: status is GRPC_CHANNEL_SHUTDOWN");
                break;
            }
        }

        while (reader->Read(&multiAccount)) {
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_account_ };
                for( int i = 0 ; i < multiAccount.account_data_size() ; ++ i ) {
                    const AccountData& account = multiAccount.account_data(i);
                    _log_and_print("update account %s.%s: %.03f", account.exchange_id().c_str(), account.currency().c_str(), account.available());
                    account_.hedge_accounts_[account.exchange_id()].currencies[account.currency()].amount = account.available();
                }
            }            

            callback->on_account_update(account_);
        }

        Status status = reader->Finish();
        if (status.ok()) {
            _log_and_print("AccountUpdater rpc succeeded.");
        } else {
            _log_and_print("AccountUpdater rpc failed.");
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