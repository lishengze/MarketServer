#pragma once

#include "global_declare.h"

#include "kafka/KafkaConsumer.h"
#include "kafka/KafkaProducer.h"
#include "kafka/AdminClient.h"

#include "decode_processer.h"
#include "struct_define.h"

class KafkaServer
{
public:

    typedef kafka::clients::KafkaConsumer KConsumer;
    typedef kafka::clients::KafkaProducer KProducer;
    typedef kafka::clients::AdminClient   KAdmin;

    KafkaServer(string server_address);

    KafkaServer(DecodeProcesser* decode_processer);

    KafkaServer();

    ~KafkaServer();

    void init_user();

    void init_topic_list();

    void init_decode_processer(DecodeProcesser* decode_processer) {
        decode_processer_ = decode_processer;
    }

    void launch();

public:
    void sub_topic(const string& topic);
    void unsub_topic(const string& topic);
    void subscribe_topics(std::set<string> topics);


    kafka::Topics _get_subed_topics();
    kafka::Topics _get_created_topics();

    //auto createResult = adminClient.createTopics({args->topic}, 
    // args->partitions, args->replicationFactor, args->topicProps);
    bool create_topic(kafka::Topic topic);

    string _get_kline_topic(string exchange, string symbol);
    string _get_depth_topic(string exchange, string symbol);
    string _get_trade_topic(string exchange, string symbol);

public:
    void start_listen_data();
    void listen_data_main();

    void start_process_data();
    void process_data();

    void set_meta(std::unordered_map<TSymbol, std::set<TExchange>>& new_sub_info);

private:
    QuoteSourceCallbackInterface*      engine_interface_{nullptr};    

    boost::shared_ptr<KConsumer>       consumer_sptr_{nullptr};
    boost::shared_ptr<KProducer>       producer_sptr_{nullptr};
    boost::shared_ptr<KAdmin>          adclient_sptr_{nullptr};

    std::string                        bootstrap_servers_;

    std::set<string>                   topic_set_;

    std::thread                        listen_thread_;
    
    std::thread                        process_thread_;
    std::mutex                         src_data_mutex_;
    std::condition_variable            src_data_cv_;
    std::vector<std::string>           src_data_vec_;

    DecodeProcesser*                   decode_processer_;
};