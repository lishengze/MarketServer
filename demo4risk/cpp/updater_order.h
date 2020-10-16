#pragma once

#include "risk_controller_define.h"

struct SOrder {
    long long order_id; // 系统内的订单唯一ID
    SDecimal price; // 下单价格
    double volume;  // 下单量
    bool is_buy; // 买卖方向
    long long begin_time; // 对冲开始时间
    long long end_time; // 对冲完成时间

    SOrder() {
        volume = 0;
        begin_time = 0;
        end_time = 0;
    }
};

struct SOrderPriceLevel {
    SDecimal price;
    double volume;

    SOrderPriceLevel() {
        volume = 0;    
    }
};

class IOrderUpdater {
public:
    virtual void on_order_update(const TSymbol& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids) = 0;
};

class SymbolOrderCenter {
public:
    SymbolOrderCenter() {}
    ~SymbolOrderCenter() {}

    bool order_add(const string& symbol, const SOrder& order, IOrderUpdater* callback) {
        orders_[order.order_id] = order;
        // 计算回调
        vector<SOrderPriceLevel> asks, bids;
        _calc_pricelevel(asks, bids);
        callback->on_order_update(symbol, order, asks, bids);
        return true;
    }

    bool order_done(const string& symbol, const SOrder& order, IOrderUpdater* callback) {
        auto iter = orders_.find(order.order_id);
        if( iter == orders_.end() )
            return false;
        orders_.erase(iter);
        // 计算回调
        vector<SOrderPriceLevel> asks, bids;
        _calc_pricelevel(asks, bids);
        callback->on_order_update(symbol, order, asks, bids);
        return true;
    }

    void _calc_pricelevel(vector<SOrderPriceLevel>& asks, vector<SOrderPriceLevel>& bids) {
        map<SDecimal, double> ask_map, bid_map;
        for( auto iter = orders_.begin() ; iter != orders_.end() ; ++iter ) {
            const SOrder& order = iter->second;
            if( order.is_buy ) {
                bid_map[order.price] += order.volume;
            } else {
                ask_map[order.price] += order.volume;
            }
        }

        // convert to vector
        for( auto iter = ask_map.begin() ; iter != ask_map.end() ; ++iter ) {
            SOrderPriceLevel level;
            level.price = iter->first;
            level.volume = iter->second;
            asks.push_back(level);
        }
        for( auto iter = bid_map.rbegin() ; iter != bid_map.rend() ; ++iter ) {
            SOrderPriceLevel level;
            level.price = iter->first;
            level.volume = iter->second;
            bids.push_back(level);
        }
    }

    unordered_map<long long, SOrder> orders_;
};

class OrderUpdater {
public:
    OrderUpdater() {}
    ~OrderUpdater() {}

    void start(IOrderUpdater* callback) {
        SOrder order;
        string symbol;
        vector<SOrderPriceLevel> asks, bids;
        callback->on_order_update(symbol, order, asks, bids);
    }
private:
    IOrderUpdater* callback_;

    bool _order_add(const string& symbol, const SOrder& order) {
        std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
        SymbolOrderCenter* ptr = _get_ordercenter(symbol);
        return ptr->order_add(symbol, order, callback_);
    }

    bool _order_done(const string& symbol, const SOrder& order) {
        std::unique_lock<std::mutex> inner_lock{ mutex_symbols_ };
        SymbolOrderCenter* ptr = _get_ordercenter(symbol);
        return ptr->order_done(symbol, order, callback_);
    }
    
    SymbolOrderCenter* _get_ordercenter(const string& symbol) {
        auto iter = symbols_.find(symbol);
        if( iter == symbols_.end() ) {
            SymbolOrderCenter* ptr = new SymbolOrderCenter();
            symbols_[symbol] = ptr;
            return ptr;
        }
        return iter->second;
    }

    mutable std::mutex                         mutex_symbols_;
    unordered_map<TSymbol, SymbolOrderCenter*> symbols_;
};
