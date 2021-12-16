#pragma once

#include "kafka/KafkaConsumer.h"
#include "kafka/KafkaProducer.h"
#include "kafka/AdminClient.h"

#include "base/cpp/base_data_stuct.h"
#include "base/cpp/basic.h"

#include "comm_type_def.h"
#include "interface_define.h"

COMM_NAMESPACE_START

class KafkaServer:public NetServer
{
public:

    typedef kafka::clients::KafkaConsumer KConsumer;
    typedef kafka::clients::KafkaProducer KProducer;
    typedef kafka::clients::AdminClient   KAdmin;

    KafkaServer(string server_address, Serializer* depth_engine);

    KafkaServer();

    ~KafkaServer();

    void init_user();

    void launch();

public:
        virtual void set_meta(const MetaType meta);
        virtual void set_meta(const MetaType depth_meta, 
                              const MetaType kline_meta, 
                              const MetaType trade_meta);

        virtual void set_kline_meta(const MetaType meta);
        virtual void set_depth_meta(const MetaType meta);
        virtual void set_trade_meta(const MetaType meta);

public:
    void sub_topic(const string& topic);
    void unsub_topic(const string& topic);
    void subscribe_topics(std::set<string> topics);

    void publish_msg(const string& topic, const string& data);


    kafka::Topics _get_subed_topics();
    kafka::Topics _get_created_topics();

    bool create_topic(kafka::Topic topic);

    void check_topic(kafka::Topic topic);

    bool filter_topic(std::set<string>& topics);

public:
    void start_listen_data();
    void listen_data_main();

    void start_process_data();
    void process_main();
    void process_data();

    struct MetaData
    {
        string      type;
        string      body;
        string      symbol;
        string      exchange;

        string simple_str()
        {
            return string("type: ") + type + ", symbol: " + symbol 
                    + ", exchange: " + exchange;
        }
    };

    bool pre_process(const string& src_data, MetaData& meta_data);

    bool is_data_subed(const TDataType& data_type, 
                       const TSymbol& symbol, 
                       const TExchange& exchange);

public:
    virtual void publish_depth(const SDepthQuote& quote);
    virtual void publish_kline(const KlineData& kline);
    virtual void publish_trade(const TradeData& trade);     

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

    std::mutex                         public_mutex_;

    std::map<TDataType, MetaType>      meta_map_;
};

DECLARE_PTR(KafkaServer);

COMM_NAMESPACE_END