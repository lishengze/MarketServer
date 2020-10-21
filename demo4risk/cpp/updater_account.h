#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
using namespace std;
#include "risk_controller_define.h"

#define MAX_CURRENCY_LENGTH 20

struct CurrencyInfo {
    char currency[MAX_SYMBOLNAME_LENGTH];
    double amount;
};

struct HedgeAccountInfo
{
    CurrencyInfo currencies[MAX_CURRENCY_LENGTH];
    int currency_length;

    HedgeAccountInfo() {
        currency_length = 0;
    }
};

struct UserAccountInfo
{
    CurrencyInfo currencies[MAX_CURRENCY_LENGTH];
    int currency_length;

    UserAccountInfo() {
        currency_length = 0;
    }
};

struct AccountInfo
{
    unordered_map<TExchange, HedgeAccountInfo> hedge_accounts_;
    UserAccountInfo user_account_;

    double get_user_amount(const string& currency) const {
        for( int i = 0 ; i < user_account_.currency_length ; ++i ) {
            if( string(user_account_.currencies[i].currency) == currency )
                return user_account_.currencies[i].amount;
        }
        return 0;
    }
    
    double get_hedge_amount(const string& currency) const {
        double total = 0;
        for( auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) {
            const HedgeAccountInfo& hedge = iter->second;
            for( int i = 0 ; i < hedge.currency_length ; ++i ) {
                if( string(hedge.currencies[i].currency) == currency ) {
                    total += hedge.currencies[i].amount;
                }
            }
        }
        return total;
    }

    void get_hedge_amounts(const string& currency, double percent, unordered_map<TExchange, double>& amounts) const {
        for( auto iter = hedge_accounts_.begin() ; iter != hedge_accounts_.end() ; ++iter ) {
            const HedgeAccountInfo& hedge = iter->second;
            for( int i = 0 ; i < hedge.currency_length ; ++i ) {
                if( string(hedge.currencies[i].currency) == currency ) {
                    amounts[iter->first] = hedge.currencies[i].amount * percent / 100;
                    break;
                }
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

    void start(IAccountUpdater* callback) {
        thread_loop_ = new std::thread(&AccountUpdater::_run, this, callback);
    }

private:
    void _run(IAccountUpdater* callback) {
        while( true ) {
            AccountInfo account;
            // 设置用户账户
            strcpy(account.user_account_.currencies[0].currency, "BTC");
            account.user_account_.currencies[0].amount = 999999;
            strcpy(account.user_account_.currencies[1].currency, "ETH");
            account.user_account_.currencies[1].amount = 999999;
            strcpy(account.user_account_.currencies[2].currency, "USDT");
            account.user_account_.currencies[2].amount = 999999999;
            account.user_account_.currency_length = 0; // 暂时不考虑用户账户
            // 设置对冲账户(火币)
            HedgeAccountInfo hedgeHuobi;
            strcpy(hedgeHuobi.currencies[0].currency, "BTC");
            hedgeHuobi.currencies[0].amount = 999999;
            strcpy(hedgeHuobi.currencies[1].currency, "ETH");
            hedgeHuobi.currencies[1].amount = 999999;
            strcpy(hedgeHuobi.currencies[2].currency, "USDT");
            hedgeHuobi.currencies[2].amount = 999999999;
            hedgeHuobi.currency_length = 3;
            account.hedge_accounts_["HUOBI"] = hedgeHuobi;
            // 设置对冲账户(okex)
            HedgeAccountInfo hedgeOkex;
            strcpy(hedgeOkex.currencies[0].currency, "BTC");
            hedgeOkex.currencies[0].amount = 999;
            strcpy(hedgeOkex.currencies[1].currency, "ETH");
            hedgeOkex.currencies[1].amount = 999;
            strcpy(hedgeOkex.currencies[2].currency, "USDT");
            hedgeOkex.currencies[2].amount = 99999;
            hedgeOkex.currency_length = 3;
            account.hedge_accounts_["OKEX"] = hedgeOkex;
            callback->on_account_update(account);
            // 定时聚合账户详情回调风控模块
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};