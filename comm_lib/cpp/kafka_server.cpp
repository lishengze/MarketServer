#include "kafka_server.h"

#include "kafka/KafkaConsumer.h"
#include "kafka/KafkaProducer.h"
#include "kafka/AdminClient.h"

#include "comm_type_def.h"
#include "util.h"
#include "comm_log.h"
#include "comm_interface_define.h"

COMM_NAMESPACE_START



KafkaServer::KafkaServer(string server_address, Serializer* serializer):
                        NetServer{serializer},
                        bootstrap_servers_{server_address}

{
    COMM_LOG_INFO("Using KafkaServer bootstrap_servers_: " + bootstrap_servers_);

    init_user();
}            

KafkaServer::~KafkaServer()
{
    if (listen_thread_.joinable())
    {
        listen_thread_.join();
    }
}

void KafkaServer::init_user()
{
    try
    {
        kafka::Properties consumer_props ({
            {"bootstrap.servers",  bootstrap_servers_},
            {"enable.auto.commit", "true"}
        });
        consumer_sptr_ = boost::make_shared<KConsumer>(consumer_props);

        kafka::Properties producer_props ({
            {"bootstrap.servers",  bootstrap_servers_},
            {"enable.idempotence", "true"}
        });
        producer_sptr_ = boost::make_shared<KProducer>(producer_props);

        kafka::Properties adclient_props ({
            {"bootstrap.servers",  bootstrap_servers_},
            {"client.id", "test_bcts"}            
        });                
        adclient_sptr_ = boost::make_shared<KAdmin>(adclient_props);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void KafkaServer::launch()
{
    try
    {
        COMM_LOG_INFO("launch");

        start_listen_data();

        start_process_data();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

kafka::Topics KafkaServer::_get_subed_topics()
{
    try
    {
        if (consumer_sptr_)
        {
            return consumer_sptr_->subscription();
        }
        else
        {
            COMM_LOG_ERROR("consumer_sptr_ nullptr");
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return kafka::Topics();
}

kafka::Topics KafkaServer::_get_created_topics()
{
    try
    {
        if (adclient_sptr_)
        {
            auto listResult = adclient_sptr_->listTopics();

            if (listResult.error)
            {
                COMM_LOG_ERROR(listResult.error.message());
            }
            else
            {
                return listResult.topics;
            }
        }
        else
        {
            COMM_LOG_ERROR("adclient_sptr_ nullptr");
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }   

    return kafka::Topics();
}

bool KafkaServer::create_topic(kafka::Topic topic)
{
    try
    {
        if (adclient_sptr_)
        {
            // kafka::Properties topic_props ({
            //     {"max.message.bytes",  "1048588"}
            // });

            kafka::Properties topic_props;            

            auto createResult = adclient_sptr_->createTopics({topic}, 3, 3, topic_props);
            if (createResult.error)
            {
                COMM_LOG_ERROR("Create Topic Error: " + createResult.error.message());
                return false;
            }
            return true;
        }
        else
        {
            COMM_LOG_ERROR("adclient_sptr_ nullptr");
        }
        return false;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
        return false;
    }
}

void KafkaServer::publish_msg(const string& topic, const string& data)
{
    try
    {
        std::lock_guard<std::mutex> lk(public_mutex_);

        check_topic(topic);

        if (producer_sptr_)
        {
            kafka::Key key{topic.c_str()};
            auto record = kafka::clients::producer::ProducerRecord(topic,
                                                   key,
                                                   kafka::Value(data.c_str(), data.size()));

            // COMM_LOG_INFO(record.topic() + ": " + record.value().toString());

            producer_sptr_->send(record,                         
                            [](const kafka::clients::producer::RecordMetadata& metadata, 
                                const kafka::Error& error) 
                            {
                                if (!error) 
                                {
                                    // COMM_LOG_INFO(metadata.toString() + " Message delivered ");
                                } 
                                else 
                                {
                                    COMM_LOG_ERROR(error.message() + " Message delivery failed");
                                }
                          },

                          KProducer::SendOption::ToCopyRecordValue);
        }
        else
        {
            COMM_LOG_ERROR("producer_sptr_ is empty");
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::start_listen_data()
{
    try
    {
        listen_thread_ = std::thread(&KafkaServer::listen_data_main, this);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::listen_data_main()
{
    try
    {
        COMM_LOG_INFO("listen_data_main");
        while (true) 
        {            
            if (!consumer_sptr_)
            {
                COMM_LOG_ERROR("consumer_sptr_ is empty");
                return;
            }

            auto records = consumer_sptr_->poll(std::chrono::milliseconds(100));

            // if (records.size()>0)
            // {
            //     COMM_LOG_INFO("records.size: " + std::to_string(records.size()));
            // }
            
            std::unique_lock<std::mutex> lk(src_data_mutex_);
            for (const auto& record: records) 
            {                
                if (!record.error()) 
                {
                    string ori_data = record.value().toString();
                    string topic = record.topic();      
                    ori_data = topic + TOPIC_SEPARATOR + ori_data;

                    // COMM_LOG_INFO(ori_data);
                    
                    src_data_vec_.emplace_back(std::move(ori_data));
                } 
                else 
                {
                    COMM_LOG_WARN(record.toString());
                }
            }

            if (src_data_vec_.size() > 0)
            {
                // COMM_LOG_INFO("src_data_vec_.size: " + std::to_string(src_data_vec_.size()));
                src_data_cv_.notify_all();   
            }                             
        }        
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::start_process_data()
{
    try
    {
        process_thread_ = std::thread(&KafkaServer::process_main, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void KafkaServer::process_main() 
{
    try
    {
        COMM_LOG_INFO("process_main");
        while(true)
        {
            std::unique_lock<std::mutex> lk(src_data_mutex_);
            src_data_cv_.wait(lk, [&](){
                return !(src_data_vec_.size()==0);
            });            

            // COMM_LOG_INFO("src_data_vec_.size: " + std::to_string(src_data_vec_.size()));

            if (serializer_) process_data();

            src_data_vec_.clear();
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::process_data()
{
    try
    {
        for (auto src_data:src_data_vec_)
        {
            MetaData meta_data;
            if (!pre_process(src_data, meta_data)) continue;

            if (!is_data_subed(meta_data.type, meta_data.symbol, meta_data.exchange)) continue;

            if (meta_data.type == DEPTH_TYPE)
            {
                serializer_->on_snap(meta_data.body);
            }
            else if (meta_data.type == KLINE_TYPE)
            {
                serializer_->on_kline(meta_data.body);
            }         
            else if (meta_data.type == TRADE_TYPE)
            {
                serializer_->on_trade(meta_data.body);
            }                  
            else 
            {
                COMM_LOG_WARN("Unknown Topic: " + (meta_data.type));
            }
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

bool KafkaServer::pre_process(const string& src_data, MetaData& meta_data)
{
    try
    {
        string::size_type topic_end_pos = src_data.find(TOPIC_SEPARATOR);
        if (topic_end_pos == std::string::npos)
        {
            COMM_LOG_WARN("Cann't Locate TOPIC_SEPARATOR " + TOPIC_SEPARATOR + ", SrCData: " + src_data);
            return false;
        }
        string topic = src_data.substr(0, topic_end_pos);
        
        std::string::size_type type_end_pos = topic.find(TYPE_SEPARATOR);
        if( type_end_pos == std::string::npos )
        {
            COMM_LOG_WARN("Cann't Locate TYPE_SEPARATOR " + TYPE_SEPARATOR + ", topic: " + topic);
            return false;            
        }
        meta_data.type = topic.substr(0, type_end_pos);

        string symbol_exchange = topic.substr(type_end_pos+1);

        std::string::size_type symbol_end_pos = symbol_exchange.find(SYMBOL_EXCHANGE_SEPARATOR);
        if( symbol_end_pos == std::string::npos)
        {
            COMM_LOG_WARN("Cann't Locate SYMBOL_EXCHANGE_SEPARATOR " + SYMBOL_EXCHANGE_SEPARATOR + ", symbol_exchange: " + symbol_exchange);
            return false;
        }
            
        meta_data.symbol = symbol_exchange.substr(0, symbol_end_pos);
        meta_data.exchange = symbol_exchange.substr(symbol_end_pos + 1);                
        meta_data.body = src_data.substr(topic_end_pos+1);

        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    return false;
}

bool KafkaServer::is_data_subed(const TDataType& data_type, 
                                const TSymbol& symbol, 
                                const TExchange& exchange)
{
    try
    {
        if (meta_map_.find(data_type) == meta_map_.end()
        || meta_map_[data_type].find(symbol) == meta_map_[data_type].end()
        || meta_map_[data_type][symbol].find(exchange) == meta_map_[data_type][symbol].end())
            return false;
        return true;
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    

    return false;
}                                

void KafkaServer::publish_depth(const SDepthQuote& depth)
{
    try
    {
        string serializer_data{std::move(serializer_->on_snap(depth))};
        
        string topic = get_depth_topic(depth.exchange, depth.symbol);

        // COMM_LOG_INFO(topic + ": " + serializer_data);

         COMM_LOG_OUTPUT_DEPTH(depth.meta_str(), depth);

        publish_msg(topic, serializer_data);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

void KafkaServer::publish_kline(const KlineData& kline)
{
    try
    {
        string serializer_data{std::move(serializer_->on_kline(kline))};
        string topic = get_kline_topic(kline.exchange, kline.symbol);

        COMM_LOG_OUTPUT_KLINE(kline.meta_str(), kline);

        publish_msg(topic, serializer_data);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }   
}

void KafkaServer::publish_trade(const TradeData& trade)
{
    try
    {
        string serializer_data{std::move(serializer_->on_trade(trade))};
        string topic = get_trade_topic(trade.exchange, trade.symbol);

        COMM_LOG_OUTPUT_TRADE(trade.meta_str(), trade);

        publish_msg(topic, serializer_data);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }       
} 


void KafkaServer::check_topic(kafka::Topic topic)
{
    try
    {
        if (created_topics_.size() == 0)
        {
            created_topics_ = _get_created_topics();
        }

        if (created_topics_.find(topic) == created_topics_.end())
        {
            COMM_LOG_INFO("topic " + topic + " was not created");

            create_topic(topic);
            created_topics_ = _get_created_topics();

            COMM_LOG_INFO("After Create: " + topic);
            for (auto topic:created_topics_)
            {
                COMM_LOG_INFO(topic);
            }            
        }
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

bool KafkaServer::filter_topic(std::set<string>& topics)
{
    try
    {
        if (created_topics_.size() == 0)
        {
            created_topics_ = _get_created_topics();
        }        

        for (auto topic:created_topics_)
        {
            COMM_LOG_INFO("created topic: " + topic);
        }

        kafka::Topics valid_topics;
        for(auto topic:topics)
        {
            
            if (created_topics_.find(topic) == created_topics_.end())
            {
                COMM_LOG_INFO("topic " + topic + " was not created");
            }
            else
            {
                valid_topics.emplace(topic);
            }
        }

        topics.swap(valid_topics);      


        std::set<string> subed_topics = _get_subed_topics();

        for (auto topic:topics)
        {
            if (subed_topics.find(topic) == subed_topics.end()) return true;
        }

        return false;

    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }

    return false;
}

void KafkaServer::sub_topic(const string& topic)
{
    try
    {
        kafka::Topics subed_topics = _get_subed_topics();
        kafka::Topics created_topics = _get_created_topics();

        if (subed_topics.find(topic) != subed_topics.end())
        {
            COMM_LOG_WARN("topic " + topic + " has subed");
            return;
        }

        if (created_topics.find(topic) == created_topics.end())
        {
            COMM_LOG_ERROR("topic " + topic + " was not created");
            return;    
        }

        consumer_sptr_->subscribe({topic});
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::unsub_topic(const string& topic)
{
    try
    {
        kafka::Topics subed_topics = _get_subed_topics();
        kafka::Topics created_topics = _get_created_topics();

        if (subed_topics.find(topic) == subed_topics.end())
        {
            COMM_LOG_INFO("topic " + topic + " was not subed");
            return;
        }

        if (created_topics.find(topic) == created_topics.end())
        {
            COMM_LOG_ERROR("topic " + topic + " was not created");
            return;    
        }

        consumer_sptr_->unsubscribe();
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
}

void KafkaServer::subscribe_topics(std::set<string> topics)
{
    try
    {
        if (!filter_topic(topics)) return;

        for(auto topic:topics)
        {
            COMM_LOG_INFO("Sub Topic: " + topic);
        }

        if (consumer_sptr_)
        {
            consumer_sptr_->unsubscribe();
            consumer_sptr_->subscribe(topics);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}

void KafkaServer::set_meta(const MetaType meta)
{
    try
    {
        std::set<string> new_topics;
        for (auto iter:meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string kline_topic = get_kline_topic(exchange, symbol);
                string depth_topic = get_depth_topic(exchange, symbol);
                string trade_topic = get_trade_topic(exchange, symbol);

                new_topics.emplace(std::move(kline_topic));
                new_topics.emplace(std::move(depth_topic));      
                new_topics.emplace(std::move(trade_topic));                 
            }                 
        }

        meta_map_[DEPTH_TYPE] = meta;
        meta_map_[KLINE_TYPE] = meta;
        meta_map_[TRADE_TYPE] = meta;

        subscribe_topics(new_topics);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }
    
}

void KafkaServer::set_meta(const MetaType depth_meta, const MetaType kline_meta,const MetaType trade_meta)
{
    try
    {
        std::set<string> new_topics;
        for (auto iter:kline_meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string topic = get_kline_topic(exchange, symbol);

                new_topics.emplace(std::move(topic));
            }
                 
        }

        for (auto iter:depth_meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string topic = get_depth_topic(exchange, symbol);

                new_topics.emplace(std::move(topic));
            }
                 
        }

        for (auto iter:trade_meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string topic = get_trade_topic(exchange, symbol);

                new_topics.emplace(std::move(topic));
            }
                 
        }      

        meta_map_[DEPTH_TYPE] = depth_meta;
        meta_map_[KLINE_TYPE] = kline_meta;
        meta_map_[TRADE_TYPE] = trade_meta;

        subscribe_topics(new_topics);  
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

void KafkaServer::set_kline_meta(const MetaType meta)
{
    try
    {
        std::set<string> new_topics;
        for (auto iter:meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string kline_topic = get_kline_topic(exchange, symbol);

                new_topics.emplace(std::move(kline_topic));
            }
                 
        }
        meta_map_[KLINE_TYPE] = meta;
        subscribe_topics(new_topics);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

void KafkaServer::set_depth_meta(const MetaType meta)
{
    try
    {
        std::set<string> new_topics;
        for (auto iter:meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string topic = get_depth_topic(exchange, symbol);

                new_topics.emplace(std::move(topic));
            }
                 
        }
        meta_map_[DEPTH_TYPE] = meta;
        subscribe_topics(new_topics);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

void KafkaServer::set_trade_meta(const MetaType meta)
{
    try
    {
        std::set<string> new_topics;
        for (auto iter:meta)
        {
            for (auto exchange:iter.second)
            {
                string symbol = iter.first;
                string topic = get_trade_topic(exchange, symbol);

                new_topics.emplace(std::move(topic));
            }
                 
        }
        meta_map_[TRADE_TYPE] = meta;
        subscribe_topics(new_topics);
    }
    catch(const std::exception& e)
    {
        COMM_LOG_ERROR(e.what());
    }    
}

COMM_NAMESPACE_END