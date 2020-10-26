#pragma once

#include "stream_engine_define.h"
#include "grpc_server.h"

/*
算法：源头的行情更新全部更新到品种的买卖盘队列中，由独立线程计算买卖盘分水岭位置，保持分水岭相对稳定可用，在发布聚合行情的时候，再基于分水岭位置进行行情价位过滤
源头更新回调：全量更新、增量更新都需要加锁
计算线程：快照数据全量加锁
*/
class QuoteMixer2
{
public:
    QuoteMixer2();
    ~QuoteMixer2();
    
    void on_snap(const string& exchange, const string& symbol, const SDepthQuote& quote);

    void on_update(const string& exchange, const string& symbol, const SDepthQuote& quote);

private:
    // 独立线程计算watermark
    mutable std::mutex mutex_snaps_;
    unordered_map<TSymbol, SDecimal> watermark_;
    unordered_map<TSymbol, unordered_map<TExchange, SDepthQuote>> snaps_;
    std::thread*               thread_loop_ = nullptr;
    void _calc_watermark();
    void _one_round();
    bool _get_watermark(const string& symbol, SDecimal& watermark) const;

    // symbols
    unordered_map<TSymbol, SMixQuote*> symbols_;

    // 发布聚合行情
    mutable std::mutex mutex_clocks_;
    unordered_map<TSymbol, long long> last_clocks_;
    bool _check_update_clocks(const string& symbol);
    void _publish_quote(const string& symbol, const SMixQuote* quote, bool isSnap);

    // 发布聚合行情（用于对冲）
    mutable std::mutex mutex_hedgeclocks_;
    unordered_map<TSymbol, long long> last_hedgeclocks_;
    bool _check_update_hedgeclocks(const string& symbol);
    void _publish_hedgequote(const string& symbol, const SMixQuote* quote, bool isSnap);

    bool _get_quote(const string& symbol, SMixQuote*& ptr) const;

    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, 
        const int& newLength, bool isAsk);

    SMixDepthPrice* _clear_exchange(const string& exchange, SMixDepthPrice* depths);

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, bool isAsk);
};