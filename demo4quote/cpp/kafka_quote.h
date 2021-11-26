#pragma once

#include "global_declare.h"
#include "quote_define.h"

class KafkaQuote
{
public:
    KafkaQuote(string server_address);

    void init_engine(QuoteSourceCallbackInterface* engine) { engine_interface_ = engine;}

    void start_listen_data();
    void listen_data_main();

private:
    QuoteSourceCallbackInterface*      engine_interface_{nullptr};    
    std::thread                        listen_thread_;

};