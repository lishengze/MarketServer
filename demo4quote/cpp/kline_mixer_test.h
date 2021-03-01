#include "kline_mixer.h"

void test_kline_calculator60()
{
    string symbol = "BTC_USDT";
    unordered_set<TExchange> exchanges;
    exchanges.insert("HUOBI");
    exchanges.insert("OKEX");
    exchanges.insert("BINANCE");

    MixCalculator calculator;
    calculator.set_symbol(symbol, exchanges);

    KlineData kline;
    vector<KlineData> klines, output;

    klines.clear();
    kline.index = 1611614520;
    kline.px_open.from("31595.4");
    kline.px_high.from("31609.0");
    kline.px_low.from("31591.1");
    kline.px_close.from("31607.5");
    kline.volume.from("0.153918");
    klines.push_back(kline);
    calculator.add_kline("HUOBI", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611614520;
    kline.px_open.from("31602.87");
    kline.px_high.from("31619.83");
    kline.px_low.from("31601.27");
    kline.px_close.from("31617.50");
    kline.volume.from("2.469071");
    klines.push_back(kline);
    calculator.add_kline("OKEX", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611614520;
    kline.px_open.from("31603.97");
    kline.px_high.from("31618.04");
    kline.px_low.from("31603.96");
    kline.px_close.from("31618.03");
    kline.volume.from("17.453984");
    klines.push_back(kline);
    calculator.add_kline("BINANCE", symbol, klines, output);
    assert( output.size() == 1 && output[0].index == 1611614520 && output[0].px_open.get_str_value() == "31603.76" && 
        output[0].px_high.get_str_value() == "31618.18" && output[0].px_low.get_str_value() == "31603.52" && 
        output[0].px_close.get_str_value() == "31617.88" && output[0].volume.get_str_value() == "20.076973" );

    klines.clear();
    kline.index = 1611614580;
    kline.px_open.from("31595.4");
    kline.px_high.from("31609.0");
    kline.px_low.from("31591.1");
    kline.px_close.from("31607.5");
    kline.volume.from("0.153918");
    klines.push_back(kline);
    calculator.add_kline("HUOBI", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611614580;
    kline.px_open.from("31602.87");
    kline.px_high.from("31619.83");
    kline.px_low.from("31601.27");
    kline.px_close.from("31617.50");
    kline.volume.from("2.469071");
    klines.push_back(kline);
    calculator.add_kline("OKEX", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611614580;
    kline.px_open.from("31603.97");
    kline.px_high.from("31618.04");
    kline.px_low.from("31603.96");
    kline.px_close.from("31618.03");
    kline.volume.from("17.453984");
    klines.push_back(kline);
    calculator.add_kline("BINANCE", symbol, klines, output);
    assert( output.size() == 1 && output[0].index == 1611614580 && output[0].px_open.get_str_value() == "31603.76" && 
        output[0].px_high.get_str_value() == "31618.18" && output[0].px_low.get_str_value() == "31603.52" && 
        output[0].px_close.get_str_value() == "31617.88" && output[0].volume.get_str_value() == "20.076973" );

}

void test_kline_calculator3600()
{
    string symbol = "BTC_USDT";
    unordered_set<TExchange> exchanges;
    exchanges.insert("HUOBI");
    exchanges.insert("OKEX");
    exchanges.insert("BINANCE");

    MixCalculator calculator;
    calculator.set_symbol(symbol, exchanges);

    KlineData kline;
    vector<KlineData> klines, output;

    klines.clear();
    kline.index = 1611662400;
    kline.px_open.from("31595.4");
    kline.px_high.from("31609.0");
    kline.px_low.from("31591.1");
    kline.px_close.from("31607.5");
    kline.volume.from("0.153918");
    klines.push_back(kline);
    calculator.add_kline("HUOBI", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611662400;
    kline.px_open.from("31602.87");
    kline.px_high.from("31619.83");
    kline.px_low.from("31601.27");
    kline.px_close.from("31617.50");
    kline.volume.from("2.469071");
    klines.push_back(kline);
    calculator.add_kline("OKEX", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611662400;
    kline.px_open.from("31603.97");
    kline.px_high.from("31618.04");
    kline.px_low.from("31603.96");
    kline.px_close.from("31618.03");
    kline.volume.from("17.453984");
    klines.push_back(kline);
    calculator.add_kline("BINANCE", symbol, klines, output);
    assert( output.size() == 1 && output[0].index == 1611662400 && output[0].px_open.get_str_value() == "31603.76" && 
        output[0].px_high.get_str_value() == "31618.18" && output[0].px_low.get_str_value() == "31603.52" && 
        output[0].px_close.get_str_value() == "31617.88" && output[0].volume.get_str_value() == "20.076973" );

    klines.clear();
    kline.index = 1611666000;
    kline.px_open.from("31595.4");
    kline.px_high.from("31609.0");
    kline.px_low.from("31591.1");
    kline.px_close.from("31607.5");
    kline.volume.from("0.153918");
    klines.push_back(kline);
    calculator.add_kline("HUOBI", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611666000;
    kline.px_open.from("31602.87");
    kline.px_high.from("31619.83");
    kline.px_low.from("31601.27");
    kline.px_close.from("31617.50");
    kline.volume.from("2.469071");
    klines.push_back(kline);
    calculator.add_kline("OKEX", symbol, klines, output);
    assert( output.size() == 0 );

    klines.clear();
    kline.index = 1611666000;
    kline.px_open.from("31603.97");
    kline.px_high.from("31618.04");
    kline.px_low.from("31603.96");
    kline.px_close.from("31618.03");
    kline.volume.from("17.453984");
    klines.push_back(kline);
    calculator.add_kline("BINANCE", symbol, klines, output);
    assert( output.size() == 1 && output[0].index == 1611666000 && output[0].px_open.get_str_value() == "31603.76" && 
        output[0].px_high.get_str_value() == "31618.18" && output[0].px_low.get_str_value() == "31603.52" && 
        output[0].px_close.get_str_value() == "31617.88" && output[0].volume.get_str_value() == "20.076973" );

}

class FakeDB : public IDataProvider
{
    // 外部请求K线
    // resolution: 60 - min1 / 3600 - min60
    // start_time / end_time: 必须为秒数，不接受其他输入
    virtual bool get_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, type_tick start_time, type_tick end_time, vector<KlineData>& klines)
    {
        return true;
    }
    // 收到K线更新
    // resolution: 60 - min1 / 3600 - min60
    // is_init：是否第一次更新
    virtual void on_kline(const TExchange& exchange, const TSymbol& symbol, int resolution, const vector<KlineData>& klines, bool is_init){
        if( resolution == 3600 ) {
            if( is_init ) {
                klines[0].print_debug();
                klines[1].print_debug();

/*
                assert( klines[0].index == 1614045600 );
                assert( klines[0].px_open.get_str_value() == "1.97" );
                assert( klines[0].px_high.get_str_value() == "4.04" );
                assert( klines[0].px_low.get_str_value() == "0.96" );
                assert( klines[0].px_close.get_str_value() == "1.97" );
                assert( klines[0].volume.get_str_value() == "3.3" );
*/
                assert( klines[0].index == 1614049200 );
                assert( klines[0].px_open.get_str_value() == "2.97" );
                assert( klines[0].px_high.get_str_value() == "4.97" );
                assert( klines[0].px_low.get_str_value() == "1.97" );
                assert( klines[0].px_close.get_str_value() == "3.97" );
                assert( klines[0].volume.get_str_value() == "1.1" );
            } else {
                assert( klines[0].index == 1614049200 );
                assert( klines[0].px_open.get_str_value() == "2.97" );
                assert( klines[0].px_high.get_str_value() == "10.97" );
                assert( klines[0].px_low.get_str_value() == "1.97" );
                assert( klines[0].px_close.get_str_value() == "5.97" );
                assert( klines[0].volume.get_str_value() == "2.2" );
            }
        }
    }
};

void test_kline_hubber()
{
    FakeDB fakedb;
    KlineHubber hubber;
    hubber.set_db_interface(&fakedb);

    vector<KlineData> klines, outputs;
    KlineData kline;
    kline.index = 1614047040;
    kline.px_open.from("1.97");
    kline.px_high.from("3.04");
    kline.px_low.from("1.96");
    kline.px_close.from("2.03");
    kline.volume.from("1.1");
    klines.push_back(kline);
    kline.index = 1614047100;
    kline.px_open.from("1.97");
    kline.px_high.from("4.04");
    kline.px_low.from("0.96");
    kline.px_close.from("1.03");
    kline.volume.from("1.1");
    klines.push_back(kline);
    kline.index = 1614047160;
    kline.px_open.from("1.97");
    kline.px_high.from("1.97");
    kline.px_low.from("1.97");
    kline.px_close.from("1.97");
    kline.volume.from("1.1");
    klines.push_back(kline);
    kline.index = 1614049200;
    kline.px_open.from("2.97");
    kline.px_high.from("4.97");
    kline.px_low.from("1.97");
    kline.px_close.from("3.97");
    kline.volume.from("1.1");
    klines.push_back(kline);
    hubber.on_kline("HUOBI", "BTC_USDT", 60, klines, true, outputs);

    klines.clear();
    outputs.clear();
    kline.index = 1614049260;
    kline.px_open.from("2.97");
    kline.px_high.from("10.97");
    kline.px_low.from("1.97");
    kline.px_close.from("5.97");
    kline.volume.from("1.1");
    klines.push_back(kline);
    hubber.on_kline("HUOBI", "BTC_USDT", 60, klines, false, outputs);
}