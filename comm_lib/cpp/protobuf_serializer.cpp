#include "base/cpp/base_data_stuct.h"
#include "protobuf_serializer.h"
#include "base/cpp/util.h"

#include "depth_processor.h"
#include "trade_processor.h"
#include "kline_processor.h"

#include "util.h"
#include "base/cpp/quote.h"


// google::protobuf::Map<std::string, PDecimal>

COMM_NAMESPACE_START



inline void set_decimal(PDecimal* dst, const SDecimal& src)
{
    dst->set_precise(src.prec());
    dst->set_value(src.value());
}

void set_depth(PRepeatedDepth* depth_list,  const map<SDecimal, SDepth>& src)
{
    try
    {
        PDepth depth;
        for (auto iter:src)
        {
            SDecimal price = iter.first;

            set_decimal(depth.mutable_price(), price);
            set_decimal(depth.mutable_volume(), iter.second.volume);
            
            PDepthMap* volume_by_exchanges = depth.mutable_volume_by_exchanges();

            for (auto iter2:iter.second.volume_by_exchanges)
            {
                string symbol = iter2.first;
                PDecimal pvolume;
                set_decimal(&pvolume, iter2.second);

                (*volume_by_exchanges)[symbol] = pvolume;

                // volume_by_exchanges->insert(symbol, pvolume);
                // set_decimal(volume_by_exchangesiter2.first], iter2.second);
            }

            depth.Clear();
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}


string ProtobufSerializer::on_snap(const SDepthQuote& depth)
{
    try
    {
        PDepthQuote quote;
        quote.set_exchange(depth.exchange);
        quote.set_symbol(depth.symbol);
        quote.set_sequence_no(depth.sequence_no);
        quote.set_origin_time(depth.origin_time);
        quote.set_arrive_time(depth.arrive_time);
        quote.set_server_time(depth.server_time);
        quote.set_price_precise(depth.price_precise);
        quote.set_volume_precise(depth.volume_precise);
        quote.set_amount_precise(depth.amount_precise);
        quote.set_is_snap(depth.is_snap);

        set_depth(quote.mutable_asks(), depth.asks);
        set_depth(quote.mutable_bids(), depth.bids);

        return quote.SerializeAsString();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }

    return "";
}

string ProtobufSerializer::on_kline(const KlineData& kline)
{
    try
    {
        PKlineData proto_kline;
        proto_kline.set_time(kline.index);
        proto_kline.set_exchange(kline.exchange);
        proto_kline.set_symbol(kline.symbol);

        set_decimal(proto_kline.mutable_px_open(), kline.px_open);
        set_decimal(proto_kline.mutable_px_high(), kline.px_high);
        set_decimal(proto_kline.mutable_px_low(), kline.px_low);
        set_decimal(proto_kline.mutable_px_close(), kline.px_close);
        set_decimal(proto_kline.mutable_volume(), kline.volume);

        proto_kline.set_resolution(kline.resolution);

        return proto_kline.SerializeAsString();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
    return "";
}

string ProtobufSerializer::on_trade(const TradeData& trade)
{
    try
    {
        PTradeData proto_trade;
        proto_trade.set_time(trade.time);
        set_decimal(proto_trade.mutable_price(), trade.price);
        set_decimal(proto_trade.mutable_volume(), trade.volume);

        proto_trade.set_symbol(trade.symbol);
        proto_trade.set_exchange(trade.exchange);

        return proto_trade.SerializeAsString();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
    return "";
}

// 行情接口
void ProtobufSerializer::on_snap(const string& src) 
{
    try
    {
        PDepthQuote proto_quote;
        if (proto_quote.ParseFromString(src))
        {
            SDepthQuote depth_quote;



            p_depth_processor_->on_snap(depth_quote);
        }
       else
        {
            COMM_LOG_WARN("decode depth faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
};

// K线接口
void ProtobufSerializer::on_kline(const string& src)
{
    try
    {
        PKlineData proto_kline;
        if (proto_kline.ParseFromString(src))
        {
            KlineData kline;
            p_kline_processor_->on_kline(kline);
            // COMM_LOG_INFO(kline.get_json_str());
        }
        else
        {
            COMM_LOG_WARN("decode kline faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

// 交易接口
void ProtobufSerializer::on_trade(const string& src)
{
    try
    {
        PTradeData proto_trade;
        if (proto_trade.ParseFromString(src))
        {
            TradeData trade_data;
            // COMM_LOG_INFO(trade_data.get_json_str());
            p_trade_processor_->on_trade(trade_data);
        }
        else
        {
            COMM_LOG_WARN("decode trade faild, ori_msg: " + src);
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void decode_depth_map(const PRepeatedDepth& src, map<SDecimal, SDepth>& dst)
{
    try
    {
        for (auto iter: src)
        {
            SDecimal price(iter.price().value(), iter.price().precise());

            SDecimal volume(iter.volume().value(), iter.volume().precise());

            const PDepthMap& volume_by_exchanges = iter.volume_by_exchanges();

            SDepth new_depth;
            new_depth.volume = volume;

            for (auto iter2:volume_by_exchanges)
            {
                SDecimal exchange_volume(iter2.second.value(), iter2.second.precise());
                new_depth.volume_by_exchanges[iter2.first] = exchange_volume;
            }

            dst[price] = new_depth;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

bool ProtobufSerializer::decode_depth(PDepthQuote& src, SDepthQuote& depth_quote)
{
    try
    {
        depth_quote.exchange = src.exchange();
        depth_quote.symbol = src.symbol();
        depth_quote.sequence_no = src.sequence_no();
        depth_quote.origin_time = src.origin_time();
        depth_quote.server_time = src.server_time();
        depth_quote.price_precise = src.price_precise();
        depth_quote.amount_precise = src.amount_precise();
        depth_quote.is_snap = src.is_snap();

        decode_depth_map(src.asks(), depth_quote.asks);
        decode_depth_map(src.bids(), depth_quote.bids);

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool ProtobufSerializer::decode_kline(PKlineData& src, KlineData& kline)
{
    try
    {
        kline.index = src.time();
        kline.exchange = src.exchange();
        kline.symbol = src.symbol();
        
        kline.resolution = src.resolution();

        kline.volume.parse_by_raw(src.volume().value(), src.volume().precise());
        kline.px_open.parse_by_raw(src.px_open().value(), src.px_open().precise());
        kline.px_high.parse_by_raw(src.px_high().value(), src.px_high().precise());
        kline.px_low.parse_by_raw(src.px_low().value(), src.px_low().precise());
        kline.px_close.parse_by_raw(src.px_close().value(), src.px_close().precise());        

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool ProtobufSerializer::decode_trade(PTradeData& src,TradeData& trade_data)
{
    try
    {
        trade_data.time = src.time();
        trade_data.symbol = src.symbol();
        trade_data.exchange = src.exchange();

        trade_data.price.parse_by_raw(src.price().value(), src.price().precise());
        trade_data.volume.parse_by_raw(src.volume().value(), src.volume().precise());

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

COMM_NAMESPACE_END