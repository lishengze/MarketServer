#pragma once

#include "global_declare.h"

#include "kafka/KafkaConsumer.h"
#include "kafka/KafkaProducer.h"

#include "decode_processer.h"

class KafkaQuote
{
public:

    typedef kafka::clients::KafkaConsumer KConsumer;
    typedef kafka::clients::KafkaProducer KProducer;

    KafkaQuote(string server_address);

    KafkaQuote(DecodeProcesser* decode_processer):
        decode_processer_{decode_processer}
    {

    }

    KafkaQuote();

    ~KafkaQuote();

    void init_user();

    void init_topic_list();

    void sub_topics();

    void init_decode_processer(DecodeProcesser* decode_processer) {
        decode_processer_ = decode_processer;
    }

    void launch();

    void start_listen_data();
    void listen_data_main();

    void start_process_data();
    void process_data();

private:
    QuoteSourceCallbackInterface*      engine_interface_{nullptr};    

    boost::shared_ptr<KConsumer>       consumer_sptr_{nullptr};
    boost::shared_ptr<KProducer>       producer_sptr_{nullptr};

    std::string                        bootstrap_servers_;

    std::list<string>                  topic_list_;

    std::thread                        listen_thread_;
    
    std::thread                        process_thread_;
    std::mutex                         src_data_mutex_;
    std::condition_variable            src_data_cv_;
    std::vector<std::string>           src_data_vec_;

    DecodeProcesser*                   decode_processer_;
};