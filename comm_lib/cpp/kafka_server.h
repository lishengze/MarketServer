#pragma once

#include "global_declare.h"

#include "kafka/KafkaConsumer.h"
#include "kafka/KafkaProducer.h"
#include "kafka/AdminClient.h"

#include "decode_processer.h"
#include "base/cpp/base_data_stuct.h"
#include "base/cpp/basic.h"

#include "comm_type_def.h"

COMM_NAMESPACE_START

class KafkaServer
{
public:

    typedef kafka::clients::KafkaConsumer KConsumer;
    typedef kafka::clients::KafkaProducer KProducer;
    typedef kafka::clients::AdminClient   KAdmin;

    KafkaServer(string server_address);

    KafkaServer(string server_address, 
                QuoteSourceCallbackInterface* depth_engine = nullptr,
                QuoteSourceCallbackInterface* kline_engine = nullptr,
                QuoteSourceCallbackInterface* trade_engine = nullptr);

    KafkaServer();

    ~KafkaServer();

    void init_user();

    void init_topic_list();

    void init_decode_processer(DecodeProcesser* decode_processer) {
        decode_processer_ = decode_processer;
    }

    void launch();

public:
        void set_meta(const MetaType kline_meta, const MetaType depth_meta, const MetaType trade_meta);

        void set_kline_meta(const MetaType meta);
        void set_depth_meta(const MetaType meta);
        void set_trade_meta(const MetaType meta);

public:
    void sub_topic(const string& topic);
    void unsub_topic(const string& topic);
    void subscribe_topics(std::set<string> topics);

    void publish_msg(const string& topic, const string& data);


    kafka::Topics _get_subed_topics();
    kafka::Topics _get_created_topics();

    bool create_topic(kafka::Topic topic);

    void check_topic(kafka::Topic topic);

    void filter_topic(std::set<string>& topics);

public:
    void start_listen_data();
    void listen_data_main();

    void start_process_data();
    void process_data();

    void set_meta(std::unordered_map<TSymbol, std::set<TExchange>>& new_sub_info);

private: 

    boost::shared_ptr<KConsumer>       consumer_sptr_{nullptr};
    boost::shared_ptr<KProducer>       producer_sptr_{nullptr};
    boost::shared_ptr<KAdmin>          adclient_sptr_{nullptr};

    std::string                        bootstrap_servers_;

    std::set<string>                   created_topics_;

    std::thread                        listen_thread_;
    
    std::thread                        process_thread_;
    std::mutex                         src_data_mutex_;
    std::condition_variable            src_data_cv_;
    std::vector<std::string>           src_data_vec_;

    DecodeProcesserPtr                 decode_processer_{nullptr};

    std::mutex                         public_mutex_;
};

DECLARE_PTR(KafkaServer);

COMM_NAMESPACE_END