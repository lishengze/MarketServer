#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
using namespace std;

struct SymbolInfo
{
    double amount;
};

struct AccountInfo
{
    unordered_map<string, unordered_map<string, SymbolInfo>> infos;
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
            account.infos["okex"]["BTC"].amount = 99.9;
            account.infos["okex"]["USDT"].amount = 99.9;
            account.infos["huobi"]["BTC"].amount = 99.9;
            account.infos["huobi"]["USDT"].amount = 99.9;
            callback->on_account_update(account);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    std::thread*               thread_loop_ = nullptr;
};