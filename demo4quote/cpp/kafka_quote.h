#pragma once

#include "global_declare.h"
#include "struct_define.h"

#include "kafka/KafkaConsumer.h"

#include "pandora/util/io_service_pool.h"

class KafkaQuote
{
public:

    typedef kafka::clients::KafkaConsumer KConsumer;

    KafkaQuote(string server_address);

    KafkaQuote();

    ~KafkaQuote();

    void init_user();

    void init_topic_list();

    void init_engine(QuoteSourceCallbackInterface* engine) { engine_interface_ = engine;}

    void launch();

    void start_listen_data();
    void listen_data_main();

    void start_process_data();
    void process_data_main() { }

    void process_data(std::vector<string> src_data_vec);

private:
    QuoteSourceCallbackInterface*      engine_interface_{nullptr};    



    boost::shared_ptr<KConsumer>       consumer_sptr_{nullptr};

    std::string                        bootstrap_servers_;

    std::list<string>                  topic_list_;

    std::thread                        process_thread_;

    int                                process_data_thread_count_;
    utrade::pandora::io_service_pool   process_pool_;

    // std::thread                        listen_thread_;
    // std::mutex                         src_data_mutex_;
    // std::condition_variable            src_data_cv_;
    // std::vector<std::string>           src_data_vec_;
};