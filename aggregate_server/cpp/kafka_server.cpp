#include "kafka_server.h"
#include "Log/log.h"
#include "stream_engine_config.h"


KafkaServer::KafkaServer(string bootstrap_servers):
     bootstrap_servers_{bootstrap_servers}
{
    init_user();
}

KafkaServer::KafkaServer()
{
    this->bootstrap_servers_ = KAFKA_CONFIG.bootstrap_servers;

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
            {"client.id", "bcts"}            
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
        start_listen_data();

        start_process_data();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
            LOG_ERROR("consumer_sptr_ nullptr");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
    
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
                LOG_ERROR(listResult.error.message());
            }
            else
            {
                return listResult.topics;
            }
        }
        else
        {
            LOG_ERROR("adclient_sptr_ nullptr");
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }   
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
                LOG_ERROR("Create Topic Error: " + createResult.error.message());
                return false;
            }
            return true;
        }
        else
        {
            LOG_ERROR("adclient_sptr_ nullptr");
        }
        return false;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
        return false;
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
        LOG_ERROR(e.what());
    }
}

void KafkaServer::listen_data_main()
{
    try
    {
        LOG_INFO("listen_data_main");
        while (true) 
        {            
            auto records = consumer_sptr_->poll(std::chrono::milliseconds(100));
            LOG_INFO("records.size: " + std::to_string(records.size()));

            std::unique_lock<std::mutex> lk(src_data_mutex_);
            for (const auto& record: records) 
            {                
                if (!record.error()) 
                {
                    string ori_data = record.value().toString();
                    string topic = record.topic();      
                    ori_data = topic + TOPIC_SEPARATOR + ori_data;

                    LOG_INFO(ori_data);
                    
                    src_data_vec_.emplace_back(std::move(ori_data));
                } 
                else 
                {
                    LOG_WARN(record.toString());
                }
            }

            if (src_data_vec_.size() > 0)
            {
                src_data_cv_.notify_all();   
            }                             
        }        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void KafkaServer::start_process_data()
{
    try
    {
        process_thread_ = std::thread(&KafkaServer::process_data, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void KafkaServer::process_data() 
{
    try
    {
        LOG_INFO("process_data");
        while(true)
        {
            std::unique_lock<std::mutex> lk(src_data_mutex_);
            src_data_cv_.wait(lk, [&](){
                return !(src_data_vec_.size()==0);
            });            

            decode_processer_->process_data(src_data_vec_);

            src_data_vec_.clear();
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string KafkaServer::_get_kline_topic(string exchange, string symbol)
{
    try
    {
        return string(KLINE_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

string KafkaServer::_get_depth_topic(string exchange, string symbol)
{
    try
    {
        return string(DEPTH_TYPE) + TYPE_SEPARATOR + symbol + SYMBOL_EXCHANGE_SEPARATOR + exchange;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void KafkaServer::sub_topic(const string& topic)
{
    try
    {
        kafka::Topics subed_topics = _get_subed_topics();
        kafka::Topics created_topics = _get_created_topics();

        if (subed_topics.find(topic) != subed_topics.end())
        {
            LOG_WARN("topic " + topic + " has subed");
            return;
        }

        if (created_topics.find(topic) == created_topics.end())
        {
            LOG_ERROR("topic " + topic + " was not created");
            return;    
        }

        consumer_sptr_->subscribe({topic});
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
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
            LOG_INFO("topic " + topic + " was not subed");
            return;
        }

        if (created_topics.find(topic) == created_topics.end())
        {
            LOG_ERROR("topic " + topic + " was not created");
            return;    
        }

        consumer_sptr_->unsubscribe();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}


void KafkaServer::subscribe_topics(std::set<string> topics)
{
    try
    {
        kafka::Topics created_topics = _get_created_topics();

        for(auto topic:topics)
        {
            if (created_topics.find(topic) == created_topics.end())
            {
                LOG_INFO("topic " + topic + " was not created");
            }
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

bool KafkaServer::set_config(const TSymbol& symbol, const SSymbolConfig& config)
{
    try
    {
        LOG_INFO("Set Config " + symbol + ":\n");

        symbol_config_[symbol] = config;

        bool new_topic;

        std::set<string> new_topics;
        for (auto iter1:symbol_config_)
        {
            for (auto iter2:iter1.second)
            {
                string symbol = iter1.first;
                string exchange = iter2.first;
                string kline_topic = _get_kline_topic(exchange, symbol);
                string depth_topic = _get_depth_topic(exchange, symbol);

                new_topics.emplace(std::move(kline_topic));
                new_topics.emplace(std::move(depth_topic));                
            }
        }

        if (topic_set_ != new_topics)
        {
            topic_set_.swap(new_topics);

            subscribe_topics(topic_set_);
        }

        return true;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}