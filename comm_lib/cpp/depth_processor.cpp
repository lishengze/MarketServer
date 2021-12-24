#include "depth_processor.h"
#include "comm_log.h"

COMM_NAMESPACE_START


void update_depth_diff(const map<SDecimal, SDepth>& update, map<SDecimal, SDepth>& dst)
{
    for( const auto& v : update ) {
        if( v.second.volume.is_zero() ) {
            auto iter = dst.find(v.first);
            if( iter != dst.end() ) {
                dst.erase(iter);
            }
        } else {
            dst[v.first] = v.second;
        }
    }
}


DepthProcessor::DepthProcessor(QuoteSourceCallbackInterface* engine):
                            engine_{engine}
{
    
}

DepthProcessor::~DepthProcessor()
{

}

bool DepthProcessor::check(SDepthQuote& src)
{
    try
    {
        // COMM_LOG_INFO(src.str());

        if (store_first_quote(src)) return false;

        if (!is_sequenced_quote(src)) return false;

        bool update_rst = false;

        if (src.is_snap)
        {
            update_rst = process_snap_quote(src);
        }
        else
        {
            update_rst = process_update_quote(src);
        }

        if (!update_rst)
        {
            if (src.is_snap) COMM_LOG_WARN(src.symbol + "." + src.exchange + " process_snap_quote failed!");
            else COMM_LOG_WARN(src.symbol + "." + src.exchange + " process_update_quote failed!");
            return false;
        }        

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

void DepthProcessor::on_snap(SDepthQuote& src)
{
    try
    {
        if (!engine_ || !check(src)) return;

        SDepthQuote& latest_quote = latest_depth_quote_[src.symbol][src.exchange];

        // if (latest_quote.symbol == "BTC_USDT")
        // {
        //     COMM_LOG_INFO(latest_quote.symbol + "." + latest_quote.exchange + ", " 
        //             + std::to_string(latest_quote.sequence_no));
        // }

        // COMM_LOG_INFO(latest_quote.str());
        
        engine_->on_snap(latest_quote);

    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }   
}

bool DepthProcessor::is_sequenced_quote(const SDepthQuote& src)
{
    try
    {
        if (latest_depth_quote_[src.symbol][src.exchange].sequence_no < src.sequence_no)
        {
            // COMM_LOG_INFO(string("src ") + src.symbol + "." + src.exchange 
            //         + ", seq_no: " + std::to_string(latest_depth_quote_[src.symbol][src.exchange].sequence_no)
            //         + ", updated seq_no: " + std::to_string(src.sequence_no));

            return true;
        }
        else
        {
            COMM_LOG_WARN(string("src ") + src.symbol + "." + src.exchange 
                    + ", seq_no: " + std::to_string(latest_depth_quote_[src.symbol][src.exchange].sequence_no)
                    + ", updated seq_no: " + std::to_string(src.sequence_no));
        }
        return false;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool DepthProcessor::store_first_quote(const SDepthQuote& src)
{
    try
    {
        if (latest_depth_quote_.find(src.symbol) == latest_depth_quote_.end()
         || latest_depth_quote_[src.symbol].find(src.exchange) 
         == latest_depth_quote_[src.symbol].end())
        {
            if (src.is_snap)
            {
                latest_depth_quote_[src.symbol][src.exchange] = src; // ??
            }
            else
            {
                COMM_LOG_WARN(src.symbol + "." + src.exchange + " is waiting for the first snap");
            }
            return true;
        }
        return false;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
    return true;
}

bool DepthProcessor::process_snap_quote(const SDepthQuote& src)
{
    try
    {
        latest_depth_quote_[src.symbol][src.exchange] = src;

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    

    return false;
}

bool DepthProcessor::process_update_quote(const SDepthQuote& src)
{
    try
    {
        merge_update(latest_depth_quote_[src.symbol][src.exchange], src);

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    

    return false;
}

void DepthProcessor::merge_update(SDepthQuote& snap, const SDepthQuote& update)
{
    try
    {
        snap.arrive_time = update.arrive_time;
        snap.server_time = update.server_time;
        snap.sequence_no = update.sequence_no;
        snap.exchange = update.exchange;
        snap.symbol = update.symbol;
        update_depth_diff(update.asks, snap.asks);
        update_depth_diff(update.bids, snap.bids);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

COMM_NAMESPACE_END

