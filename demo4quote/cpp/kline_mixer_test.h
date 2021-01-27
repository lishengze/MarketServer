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
