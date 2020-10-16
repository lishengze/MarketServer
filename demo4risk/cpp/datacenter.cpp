#include "datacenter.h"
#include "grpc_caller.h"

bool getcurrency_from_symbol(const string& symbol, string& sell_currency, string& buy_currency) {
    // 获取标的币种
    std::string::size_type pos = symbol.find("_");
    if( pos == std::string::npos) 
        return false;
    sell_currency = symbol.substr(0, pos); 
    buy_currency = symbol.substr(pos+1);
    return true;
}

void innerquote_to_msd(const SInnerQuote& quote, MarketStreamData* msd) 
{
    msd->set_symbol(quote.symbol);
    msd->set_time(quote.time);
    msd->set_time_arrive(quote.time_arrive);
    char sequence[256];
    sprintf(sequence, "%lld", quote.seq_no);
    msd->set_msg_seq(sequence);
    // 卖盘
    for( int i = 0 ; i < quote.ask_length ; ++i ) {
        const SInnerDepth& srcDepth = quote.asks[i];
        Depth* depth = msd->add_ask_depth();        
        depth->set_price(srcDepth.price.GetStrValue());
        depth->set_volume(srcDepth.total_volume);
        for( int j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_exchange(srcDepth.exchanges[j].name);
            depthData->set_size(srcDepth.exchanges[j].volume);
        }
    }
    // 买盘
    for( int i = 0 ; i < quote.bid_length ; ++i ) {
        const SInnerDepth& srcDepth = quote.bids[i];
        Depth* depth = msd->add_bid_depth();        
        depth->set_price(srcDepth.price.GetStrValue());
        depth->set_volume(srcDepth.total_volume);
        for( int j = 0 ; j < srcDepth.exchange_length ; ++j ) {
            DepthData* depthData = depth->add_data();
            depthData->set_exchange(srcDepth.exchanges[j].name);
            depthData->set_size(srcDepth.exchanges[j].volume);
        }
    }
}

DataCenter::DataCenter() {

}

DataCenter::~DataCenter() {

}

void DataCenter::add_quote(const SInnerQuote& quote)
{
    // 假如行情缓存
    Params params;
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        // 首次出现的币种
        if( datas_.find(quote.symbol) == datas_.end() ) {
            string sell_currency, buy_currency;
            if( getcurrency_from_symbol(quote.symbol, sell_currency, buy_currency) ) {
                currency_count_[sell_currency] += 1;
                currency_count_[buy_currency] += 1;
            }
        }
        datas_[quote.symbol] = quote;
        params = params_;
    }

    _publish_quote(quote, params);
};

void DataCenter::change_account(const AccountInfo& info)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_account = info;
    }

    _update_clients();
}

void DataCenter::change_configuration(const QuoteConfiguration& config)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_config = config;
    }

    _update_clients();
}

void DataCenter::change_orders(const string& symbol, const SOrder& order, const vector<SOrderPriceLevel>& asks, const vector<SOrderPriceLevel>& bids)
{
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params_.cache_order[symbol] = make_pair(asks, bids);
    }

    _update_clients(symbol);
}

void DataCenter::add_client(CallDataServeMarketStream* client)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    clients_[client] = true;
}

void DataCenter::del_client(CallDataServeMarketStream* client)
{
    // safe callback on_snap
    std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
    auto iter = clients_.find(client);
    if( iter != clients_.end() ) {
        clients_.erase(iter);
    }
}

void DataCenter::_update_clients(const TSymbol& symbol) 
{
    Params params;
    // 获取参数
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_datas_ };
        params = params_;
    }

    if( symbol == "" ) 
    {
        for( auto iter = datas_.begin() ; iter != datas_.end() ; ++iter ) 
        {
            _publish_quote(iter->second, params);
        }
    } else {
        auto iter = datas_.find(symbol);
        if( iter == datas_.end() )
            return;

        _publish_quote(iter->second, params);
    }
}

void DataCenter::_publish_quote(const SInnerQuote& quote, const Params& params) 
{    
    SInnerQuote newQuote;
    _calc_newquote(quote, params, newQuote);

    std::shared_ptr<MarketStreamData> ptrData(new MarketStreamData);
    innerquote_to_msd(newQuote, ptrData.get());
    
    // send to clients
    {
        std::unique_lock<std::mutex> inner_lock{ mutex_clients_ };
        for( auto iter = clients_.begin() ; iter != clients_.end() ; ++iter ) {
            iter->first->add_data(ptrData);
        }
    }
}

void DataCenter::_calc_newquote(const SInnerQuote& quote, const Params& params, SInnerQuote& newQuote)
{
    string symbol = quote.symbol;
    strcpy(newQuote.symbol, quote.symbol);
    newQuote.time = quote.time;
    newQuote.time_arrive = quote.time_arrive;
    newQuote.seq_no = quote.seq_no;

    // 获取币种出现次数
    string sell_currency, buy_currency;
    if( !getcurrency_from_symbol(symbol, sell_currency, buy_currency) )
        return;
    auto iter = currency_count_.find(sell_currency);
    if( iter == currency_count_.end() )
        return;
    int sell_count = iter->second;    
    auto iter2 = currency_count_.find(buy_currency);
    if( iter2 == currency_count_.end() )
        return;
    int buy_count = iter->second;
    
    // 动态调整每个品种的资金分配量
    double sell_total_amount = (params.cache_account.get_hedge_amount(sell_currency) * params.cache_config.HedgePercent / 100 + 
        params.cache_account.get_user_amount(sell_currency) * params.cache_config.UserPercent / 100) / sell_count;
    double buy_total_amount = (params.cache_account.get_hedge_amount(buy_currency) * params.cache_config.HedgePercent / 100 + 
        params.cache_account.get_user_amount(buy_currency) * params.cache_config.UserPercent / 100) / buy_count;

    // 动态调整行情价位偏置
    double volumeBias = (1 - params.cache_config.VolumeBias * 1.0/ 100);
    {
        int count = 0;
        SDecimal lastPrice = SDecimal::MinDecimal();
        for( int i = 0 ; i < quote.ask_length ; ++i )
        {
            const SInnerDepth& level = quote.asks[i];
            
            // 缩放然后向上取整
            SDecimal price = level.price * ( 1 + params.cache_config.PriceBias * 1.0 / 100);
            SDecimal scaledPrice;
            scaledPrice.From(price, -1, true); 

            if( scaledPrice > lastPrice ) {
                count++;
                newQuote.asks[count-1].price = scaledPrice;
                lastPrice = scaledPrice;
            }
            newQuote.asks[count-1].mix_exchanges(level, volumeBias);
        }
        newQuote.ask_length = count;
    }
    {
        int count = 0;
        SDecimal lastPrice = SDecimal::MaxDecimal();
        for( int i = 0 ; i < quote.bid_length ; ++i )
        {
            const SInnerDepth& level = quote.bids[i];
            
            // 缩放然后向下取整
            SDecimal price = level.price * ( 1 - params.cache_config.PriceBias * 1.0 / 100);
            SDecimal scaledPrice;
            scaledPrice.From(price, -1, false); 

            if( scaledPrice < lastPrice ) {
                count++;
                newQuote.bids[count-1].price = scaledPrice;
                lastPrice = scaledPrice;
            }
            newQuote.bids[count-1].mix_exchanges(level, volumeBias);
        }
        newQuote.bid_length = count;
    }

    // 逐档计算余额消耗量
    for( int i = 0 ; i < newQuote.ask_length ; ++i ) {
        newQuote.asks[i].amount_cost = newQuote.asks[i].total_volume;
    }
    for( int i = 0 ; i < newQuote.bid_length ; ++i ) {
        newQuote.bids[i].amount_cost = newQuote.bids[i].price.GetValue() * newQuote.bids[i].total_volume;
    }

    // 逐档从总余额中扣除资金消耗
    for( int i = 0 ; i < newQuote.ask_length ; ++i ) {
        if( sell_total_amount < newQuote.asks[i].amount_cost ) {
            newQuote.ask_length = i; // 缩短行情档位
            break;
        }
        sell_total_amount -= newQuote.asks[i].amount_cost;
    }
    for( int i = 0 ; i < newQuote.bid_length ; ++i ) {
        if( buy_total_amount < newQuote.bids[i].amount_cost ) {
            newQuote.bid_length = i; // 缩短行情档位
            break;
        }
        buy_total_amount -= newQuote.bids[i].amount_cost;
    }

    // 加工后聚合行情扣减系统未对冲单的价位
}
