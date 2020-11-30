#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
using namespace std;
#include "grpc/grpc.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "account.grpc.pb.h"
#include "google/protobuf/empty.pb.h"
#include "risk_controller_config.h"
#include "account.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
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
        check_loop_ = new std::thread(&AccountUpdater::_check_loop, this);
    }

private:
    void _request(const string& addr, IAccountUpdater* callback) {
        auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
        std::unique_ptr<Asset::Stub> stub = Asset::NewStub(channel);

        google::protobuf::Empty req;
        AccountStreamData multiAccount;
        ClientContext context;

        std::unique_ptr<ClientReader<AccountStreamData> > reader(stub->GetAccountStream(&context, req));
        switch(channel->GetState(true)) {
            case GRPC_CHANNEL_IDLE: {
                std::cout << "AccountUpdater: status is GRPC_CHANNEL_IDLE" << endl;
                break;
            }
            case GRPC_CHANNEL_CONNECTING: {                
                std::cout << "AccountUpdater: status is GRPC_CHANNEL_CONNECTING" << endl;
                break;
            }
            case GRPC_CHANNEL_READY: {           
                std::cout << "AccountUpdater: status is GRPC_CHANNEL_READY" << endl;
                break;
            }
            case GRPC_CHANNEL_TRANSIENT_FAILURE: {         
                std::cout << "AccountUpdater: status is GRPC_CHANNEL_TRANSIENT_FAILURE" << endl;
                return;
            }
            case GRPC_CHANNEL_SHUTDOWN: {        
                std::cout << "AccountUpdater: status is GRPC_CHANNEL_SHUTDOWN" << endl;
                break;
            }
        }

        while (reader->Read(&multiAccount)) {
            {
                std::unique_lock<std::mutex> inner_lock{ mutex_account_ };
                for( int i = 0 ; i < multiAccount.account_data_size() ; ++ i ) {
                    const AccountData& account = multiAccount.account_data(i);
                    _log_and_print("update account %s-%s: %.03f", account.exchange_id().c_str(), account.currency().c_str(), account.available());
                    account_.hedge_accounts_[account.exchange_id()].currencies[account.currency()].amount = account.available();
                }
            }            

            callback->on_account_update(account_);
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "AccountUpdater rpc succeeded." << std::endl;
        } else {
            std::cout << "AccountUpdater rpc failed." << std::endl;
        }
    }

    void _check_loop() const
    {
        while( 1 ) 
        {
            _println_("-------------------");
            std::unique_lock<std::mutex> inner_lock{ mutex_account_ };
            for( const auto& v : account_.hedge_accounts_ )
            {
                for( const auto& v2 : v.second.currencies ) 
                {
                    _println_("%s-%s: %.03f", v.first.c_str(), v2.first.c_str(), v2.second.amount);
                }
            }
            _println_("-------------------");

            // 休眠
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    void _run(const string& addr, IAccountUpdater* callback) {

        // 手动加入测试数据
        account_.hedge_accounts_["ART"].currencies["ABC"].amount = 99999999;
        account_.hedge_accounts_["ART"].currencies["USDT"].amount = 99999999;
        account_.hedge_accounts_["ALAMEDA"].currencies["BTC"].amount = 99999999;
        account_.hedge_accounts_["ALAMEDA"].currencies["USDT"].amount = 99999999;
        account_.hedge_accounts_["HUOBI"].currencies["BTC"].amount = 99999999;
        account_.hedge_accounts_["HUOBI"].currencies["USDT"].amount = 99999999;
        account_.hedge_accounts_["BINANCE"].currencies["BTC"].amount = 99999999;
        account_.hedge_accounts_["BINANCE"].currencies["USDT"].amount = 99999999;
        callback->on_account_update(account_);
        
        while( 1 ) {            
            _request(addr, callback);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    std::thread*               thread_loop_ = nullptr;
    std::thread*               check_loop_ = nullptr;


    mutable std::mutex         mutex_account_;
    AccountInfo                account_;
};