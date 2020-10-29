#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

#define PRICE_PRECISE 0.000000001
#define VOLUME_PRECISE 0.000000001
#define MIX_DEPTH_MAX 20
#define TSymbol string
#define TExchange string
#define CALC_BASE(x) (pow(10, (x)))

struct SDecimal {
    long long Value;
    short base;

    SDecimal() {
        memset(this, 0, sizeof(SDecimal));
    }

    void from(const string& data, int precise = -1, bool ceiling = false) {
        std::string::size_type pos = data.find(".");
        base = data.length() - pos - 1;
        if( precise >= 0 && precise < base ) { // 精度调整
            base = precise;
            string newData = data.substr(0, pos + 1 + precise);
            Value = atof(newData.c_str()) * CALC_BASE(base);
            if( ceiling )
                Value += 1;
        } else {
            Value = atof(data.c_str()) * CALC_BASE(base);
        }
    }

    void from(const SDecimal& data, int precise = -1, bool ceiling = false) {
        if( precise == -1 || data.base <= precise ) {
            Value = data.Value;
            base = data.base;
            return;
        }

        if( ceiling ) {
            Value = ceil(data.Value / CALC_BASE(data.base - precise));
        } else {
            Value = floor(data.Value / CALC_BASE(data.base - precise));
        }
        base = precise;
    }

    double get_value() const {
        return Value * 1.0 / CALC_BASE(base);
    }

    string get_str_value() const {
        char precise[25];
        sprintf(precise, "%d", base+1);

        char holder[1024];
        string fmt = "%0" + string(precise) + "lld";
        sprintf(holder, fmt.c_str(), Value);
        string ret = holder;
        ret.insert(ret.begin() + ret.length() - base, '.');
        return ret;
    }

    bool operator <(const SDecimal& d) const {
        if( base > d.base ) {
            return Value < d.Value * CALC_BASE(base-d.base);
        } else {
            return Value * CALC_BASE(d.base-base) < d.Value;
        }
    }
    bool operator >(const SDecimal& d) const {
        if( base > d.base ) {
            return Value > d.Value * CALC_BASE(base-d.base);
        } else {
            return Value * CALC_BASE(d.base-base) > d.Value;
        }
    }
    bool operator ==(const SDecimal& d) const {
        if( base > d.base ) {
            return Value == d.Value * CALC_BASE(base-d.base);
        } else {
            return Value * CALC_BASE(d.base-base) == d.Value;
        }
    }
    bool operator <=(const SDecimal& d) const {
        if( base > d.base ) {
            return Value <= d.Value * CALC_BASE(base-d.base);
        } else {
            return Value * CALC_BASE(d.base-base) <= d.Value;
        }
    }
    bool operator >=(const SDecimal& d) const {
        if( base > d.base ) {
            return Value >= d.Value * CALC_BASE(base-d.base);
        } else {
            return Value * CALC_BASE(d.base-base) >= d.Value;
        }
    }

    SDecimal operator + (const SDecimal &d) const {
        SDecimal ret;
        if( base > d.base ) {
            ret.base = base;
            ret.Value = Value + d.Value * CALC_BASE(base-d.base);
        } else {
            ret.base = d.base;
            ret.Value = Value * CALC_BASE(d.base-base) + d.Value;
        }
        return ret;
    }
    SDecimal operator - (const SDecimal &d) const {
        SDecimal ret;
        if( base > d.base ) {
            ret.base = base;
            ret.Value = Value - d.Value * CALC_BASE(base-d.base);
        } else {
            ret.base = d.base;
            ret.Value = Value * CALC_BASE(d.base-base) - d.Value;
        }
        return ret;
    }
    SDecimal operator / (const double &d) const {
        SDecimal ret;
        ret.base = base;
        ret.Value = Value / d;
        return ret;
    }
    SDecimal operator * (const double &d) const {
        SDecimal ret;
        ret.base = base;
        ret.Value = Value * d;
        return ret;
    }
};

struct SDepthPrice {
    SDecimal Price;
    double Volume;

    SDepthPrice() {
        memset(this, 0, sizeof(SDepthPrice));
    }
};

#define MAX_DEPTH 50
struct SDepthQuote {
    char exchange[32];
    char symbol[32];
    long long sequence_no;
    char time_arrive[64];
    SDepthPrice Asks[MAX_DEPTH];
    int AskLength;
    SDepthPrice Bids[MAX_DEPTH];
    int BidLength;

    SDepthQuote() {
        memset(this, 0, sizeof(SDepthQuote));
    }
};

struct SMixDepthPrice {
    SDecimal Price;
    unordered_map<string, double> Volume;
    SMixDepthPrice* Next;

    SMixDepthPrice() {
        Next = NULL;
    }
};

template<class T,class S>
inline void vassign(T &r, S v)
{
	r = v;
}

template<class T>
inline void vassign(T &r, const T v)
{
	r = v;
}

inline void vassign(char * r, char *v)
{
	strcpy(r,v);
}

inline void vassign(char * r,const char *v)
{
	strcpy(r,v);
}

inline void vassign(char * r,const std::string &v)
{
    strcpy(r,v.c_str());
}

#define MAX_MIXDEPTH 50
struct SMixQuote {
    SMixDepthPrice* Asks; // 卖盘
    SMixDepthPrice* Bids; // 买盘
    SDecimal Watermark;
    long long SequenceNo;

    SMixQuote() {
        Asks = NULL;
        Bids = NULL;
    }
};

void process_precise(const string& symbol, const SDepthQuote& src, SDepthQuote& dst) {
    int precise = 1;
    
    vassign(dst.exchange, src.exchange);
    vassign(dst.symbol, src.symbol);
    vassign(dst.sequence_no, src.sequence_no);
    vassign(dst.time_arrive, src.time_arrive);

    // sell
    {
        int count = 0;
        SDecimal lastPrice;        
        for (int i = 0 ; count < MAX_DEPTH && i < src.AskLength; ++i )
        {
            const SDecimal& price = src.Asks[i].Price;
            const double& volume = src.Asks[i].Volume;
            SDecimal d;
            d.from(price, precise, true); // 卖价往上取整
            if( d > lastPrice ) {
                count ++;
                dst.Asks[count-1].Price = d;
                lastPrice = d;
            }
            dst.Asks[count-1].Volume += volume;
        }
        dst.AskLength = count;
    }
    // buy
    {
        int count = 0;
        SDecimal lastPrice;
        lastPrice.Value = 999999999;
        for (int i = 0 ; count < MAX_DEPTH && i < src.BidLength; ++i )
        {
            const SDecimal& price = src.Bids[i].Price;
            const double& volume = src.Bids[i].Volume;
            SDecimal d;
            d.from(price, precise, false); // 买价往下取整
            if( d < lastPrice ) {
                count ++;
                dst.Bids[count-1].Price = d;
                lastPrice = d;
            }
            dst.Bids[count-1].Volume += volume;
        }
        dst.BidLength = count;
    }
}

class QuoteMixer
{
public:
public:
    QuoteMixer(){
    }

    void on_mix_snap(const string& exchange, const string& symbol, const SDepthQuote& quote) {       
        // compress price precise
        // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
        SDepthQuote cpsQuote;
        process_precise(symbol, quote, cpsQuote);

        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) ) {
            ptr = new SMixQuote();
            symbols_[symbol] = ptr;
        } else {
            // 1. 清除老的exchange数据
            ptr->Asks = _clear_exchange(exchange, ptr->Asks);
            ptr->Bids = _clear_exchange(exchange, ptr->Bids);
            // 2. 修剪cross的价位
            _cross_askbid(ptr, cpsQuote);
        }
        // 3. 合并价位
        ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
        ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);

        // 4. 推送结果
        ptr->SequenceNo = cpsQuote.sequence_no;
        publish_quote(symbol, *ptr, true);
        return;
    }

    void on_mix_update(const string& exchange, const string& symbol, const SDepthQuote& quote) {
        // compress price precise
        // 需要进行价格压缩：例如huobi的2位小数压缩为1位小数
        SDepthQuote cpsQuote;
        process_precise(symbol, quote, cpsQuote);

        SMixQuote* ptr = NULL;
        if( !_get_quote(symbol, ptr) )
            return;
        // 1. 需要清除的价位数据
        ptr->Asks = _clear_pricelevel(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, true);
        ptr->Bids = _clear_pricelevel(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, false);
        // 2. 修剪cross的价位
        _cross_askbid(ptr, cpsQuote);
        // 3. 合并价位
        ptr->Asks = _mix_exchange(exchange, ptr->Asks, cpsQuote.Asks, cpsQuote.AskLength, ptr->Watermark, true);
        ptr->Bids = _mix_exchange(exchange, ptr->Bids, cpsQuote.Bids, cpsQuote.BidLength, ptr->Watermark, false);
        // 4. 推送结果
        ptr->SequenceNo = cpsQuote.sequence_no;
        publish_quote(symbol, *ptr, false);
        return;
    }

    void display_depth(const string& prefix, const SMixDepthPrice* depth) {
        int depth_count = 0;
        vector<pair<string, unordered_map<string, double>>> depths;
        const SMixDepthPrice* ptr = depth;
        while( ptr != NULL && depth_count < MIX_DEPTH_MAX) {
            depths.push_back(make_pair(ptr->Price.get_str_value(), ptr->Volume));
            ptr = ptr->Next;
            depth_count ++;
        }

        if( prefix == "ask" ) {
            for( auto iter = depths.rbegin() ; iter != depths.rend() ; ++iter ) {
                cout << prefix << ":" << iter->first << "(";
                for( auto iter2 = iter->second.begin() ; iter2 != iter->second.end() ; ++iter2 ) {
                    cout << iter2->first << ",";
                }
                cout << ")" << endl;
            }
        } else {
            for( auto iter = depths.begin() ; iter != depths.end() ; ++iter ) {
                cout << prefix << ":" << iter->first << "(";
                for( auto iter2 = iter->second.begin() ; iter2 != iter->second.end() ; ++iter2 ) {
                    cout << iter2->first << ",";
                }
                cout << ")" << endl;
            }
        }
    };

    void publish_quote(const string& symbol, const SMixQuote& quote, bool isSnap) {
        display_depth("ask", quote.Asks);
        display_depth("bid", quote.Bids);
        cout << "---------------" << endl;
    }

private:
    // symbols
    unordered_map<TSymbol, SMixQuote*> symbols_;

    bool _get_quote(const string& symbol, SMixQuote*& ptr) const {
        auto iter = symbols_.find(symbol);
        if( iter == symbols_.end() )
            return false;
        ptr = iter->second;
        return true;
    }

    
    SMixDepthPrice* _clear_pricelevel(const string& exchange, SMixDepthPrice* depths, const SDepthPrice* newDepths, const int& newLength, bool isAsk) {
        SMixDepthPrice head;
        head.Next = depths;        
        SMixDepthPrice *tmp = depths, *last = &head;

        for( int i = 0 ; i < newLength ; ++i ) {
            const SDepthPrice& depth = newDepths[i];
            if( depth.Volume > VOLUME_PRECISE ) {
                continue;
            }
            // 找到并删除对应价位
            while( tmp != NULL ) {
                if( tmp->Price == depth.Price ) {                
                    unordered_map<TExchange, double>& volumeByExchange = tmp->Volume;
                    auto iter = volumeByExchange.find(exchange);
                    if( iter != volumeByExchange.end() ) {
                        volumeByExchange.erase(iter);                
                        if( volumeByExchange.size() == 0 ) {
                            // 删除             
                            SMixDepthPrice* waitToDel = tmp;
                            last->Next = tmp->Next;
                            tmp = tmp->Next;
                            delete waitToDel;
                            break;
                        }
                    }
                } else if ( isAsk ? (tmp->Price > depth.Price) : (tmp->Price < depth.Price) ) {
                    break;
                }
                last = tmp;
                tmp = tmp->Next;
            }
        }
        return head.Next;
    }

    SMixDepthPrice* _clear_exchange(const string& exchange, SMixDepthPrice* depths) {
        SMixDepthPrice head;
        head.Next = depths;        
        SMixDepthPrice *tmp = depths, *last = &head;
        while( tmp != NULL ) {
            unordered_map<TExchange, double>& volumeByExchange = tmp->Volume;
            auto iter = volumeByExchange.find(exchange);
            if( iter != volumeByExchange.end() ) {
                volumeByExchange.erase(iter);                
                if( volumeByExchange.size() == 0 ) {
                    // 删除             
                    SMixDepthPrice* waitToDel = tmp;
                    last->Next = tmp->Next;
                    tmp = tmp->Next;
                    delete waitToDel;
                    continue;
                }
            }
            last = tmp;
            tmp = tmp->Next;
        }
        return head.Next;
    }

    void _cross_askbid(SMixQuote* mixedQuote, const SDepthQuote& quote) {
        // 新行情的买1价 大于 聚合行情的卖1价
        if( mixedQuote->Asks != NULL && quote.BidLength > 0 && mixedQuote->Asks->Price < quote.Bids[0].Price ) {
            mixedQuote->Watermark = (quote.Bids[0].Price + mixedQuote->Asks->Price) / 2;
        }
        // 新行情的卖1价 小于 聚合行情的买1价
        else if( mixedQuote->Bids != NULL && quote.AskLength > 0 && mixedQuote->Bids->Price > quote.Asks[0].Price ) {
            mixedQuote->Watermark = (mixedQuote->Bids->Price + quote.Asks[0].Price) / 2;
        }
        else {
            mixedQuote->Watermark = SDecimal();
        }
    }

    SMixDepthPrice* _mix_exchange(const string& exchange, SMixDepthPrice* mixedDepths, const SDepthPrice* depths, 
        const int& length, const SDecimal& watermark, bool isAsk) { 
        SMixDepthPrice head;
        head.Next = mixedDepths;
        SMixDepthPrice* last = &head;

        // 1. 裁剪聚合行情对应价位（开区间）
        SMixDepthPrice* tmp = mixedDepths;
        while( tmp != NULL ) {
            if( watermark.Value == 0 || (isAsk ? (tmp->Price > watermark) : (tmp->Price < watermark)) ) {
                break;
            } else {
                SMixDepthPrice* waitToDel = tmp;
                // 删除
                last->Next = tmp->Next;       
                tmp = tmp->Next;
                delete waitToDel;
            }
        }

        // 2. 裁剪新行情对应价位（闭区间）
        int filteredLevel = 99999999;
        double virtualVolume = 0; // 虚拟价位挂单量
        for( int i = 0 ; i < length ; ++ i ) {
            const SDepthPrice& depth = depths[i];
            if( depth.Volume < VOLUME_PRECISE ) {
                continue;
            }
            if( watermark.Value == 0 || (isAsk ? (depth.Price >= watermark) : (depth.Price <= watermark)) ) {
                filteredLevel = i;
                break;        
            } else {
                virtualVolume += depth.Volume;
            }
        }

        // 3. 插入虚拟价位
        if( virtualVolume > 0 && watermark.Value > 0 ) {
            SMixDepthPrice* virtualDepth = new SMixDepthPrice();
            virtualDepth->Price = watermark;
            virtualDepth->Volume[exchange] = virtualVolume;
            // 加入链表头部
            virtualDepth->Next = head.Next;
            head.Next = virtualDepth;
        }

        // 4. 混合：first + depths
        int i = filteredLevel;
        for( tmp = head.Next ; i < length && tmp != NULL ; ) {
            const SDepthPrice& depth = depths[i];
            if( depth.Volume < VOLUME_PRECISE ) {
                ++i;
                continue;
            }
            if( isAsk ? (depth.Price < tmp->Price) : (depth.Price > tmp->Price) ) {
                // 新建价位
                SMixDepthPrice *newDepth = new SMixDepthPrice();
                newDepth->Price = depth.Price;
                newDepth->Volume[exchange] = depth.Volume;
                
                last->Next = newDepth;
                last = newDepth;
                newDepth->Next = tmp;
                ++i;
            } else if( depth.Price == tmp->Price ) {
                tmp->Volume[exchange] = depth.Volume;
                ++i;
            } else {
                last = tmp;
                tmp = tmp->Next;
            }
        }

        // 5. 剩余全部加入队尾
        for( ; i < length ; ++i) {
            const SDepthPrice& depth = depths[i];
            if( depth.Volume < VOLUME_PRECISE ) {
                continue;
            }
            // 新建价位
            SMixDepthPrice *newDepth = new SMixDepthPrice();
            newDepth->Price = depth.Price;
            newDepth->Volume[exchange] = depth.Volume;
            newDepth->Next = NULL;
            // 插入
            last->Next = newDepth;
            last = newDepth;
        }

        // 6. 删除多余的价位
        tmp = head.Next;
        int count = 0;
        while( tmp != NULL && count < MAX_MIXDEPTH) {
            tmp = tmp->Next;
            count ++;
        }
        if( tmp != NULL ) {
            SMixDepthPrice* deletePtr = tmp->Next;
            tmp->Next = NULL;
            while( deletePtr != NULL ) {
                SMixDepthPrice *waitToDelete = deletePtr;
                deletePtr = deletePtr->Next;
                delete waitToDelete;
            }
        }

        return head.Next;
    }
};

int main() {

    string p = "11781.1";
    SDecimal sd;
    sd.from(p, -1, true);
    sd = sd * 1.002;
    std::cout << sd.get_str_value() << std::endl;
    QuoteMixer m;
    FILE* f = fopen("dump.dat", "rb");
    while( 1 ) {
        char v;
        fread(&v, 1, 1, f);
        SDepthQuote quote;
        fread(&quote, sizeof(quote), 1, f);
        if( strcmp(quote.symbol, "BTC_USDT") == 0 ) {
            int i = 0;
            i++;
            if( v == 1 ) {
                m.on_mix_snap(quote.exchange, quote.symbol, quote);
            } else {
                m.on_mix_update(quote.exchange, quote.symbol, quote);
            }
            i++;
        }
    }

    return 0;
}