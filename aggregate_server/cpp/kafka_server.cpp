#include "kafka_server.h"
#include "../Log/log.h"
#include "stream_engine_config.h"


KafkaQuote::KafkaQuote(string bootstrap_servers):
     bootstrap_servers_{bootstrap_servers}
{
    init_user();

    init_topic_list();
}

KafkaQuote::KafkaQuote()
{
    this->bootstrap_servers_ = KAFKA_CONFIG.bootstrap_servers;

    init_user();

    init_topic_list();
}

KafkaQuote::~KafkaQuote()
{
    if (listen_thread_.joinable())
    {
        listen_thread_.join();
    }
}

void KafkaQuote::init_user()
{
    try
    {
        kafka::Properties props ({
            {"bootstrap.servers",  bootstrap_servers_},
            {"enable.auto.commit", "true"}
        });

        consumer_sptr_ = boost::make_shared<KConsumer>(props);

        producer_sptr_ = boost::make_shared<KProducer>(props);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void KafkaQuote::init_topic_list()
{
    try
    {
        topic_list_.push_back("trade");
        topic_list_.push_back("depth");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

void KafkaQuote::launch()
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

void KafkaQuote::sub_topics()
{
    try
    {
        kafka::Topics topics;
        for (auto topic:topic_list_)
        {
            topics.emplace(topic);
        }

        consumer_sptr_->subscribe(topics);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void KafkaQuote::start_listen_data()
{
    try
    {
        sub_topics();

        listen_thread_ = std::thread(&KafkaQuote::listen_data_main, this);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

void KafkaQuote::listen_data_main()
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
                    ori_data = topic + "|" + ori_data;

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

void KafkaQuote::start_process_data()
{
    try
    {
        process_thread_ = std::thread(&KafkaQuote::process_data, this);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}


void KafkaQuote::process_data() 
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
