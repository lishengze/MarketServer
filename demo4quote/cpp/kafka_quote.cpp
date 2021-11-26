#include "kafka_quote.h"
#include "../Log/log.h"
#include "stream_engine_config.h"


KafkaQuote::KafkaQuote(string bootstrap_servers):
     bootstrap_servers_{bootstrap_servers},
     process_data_thread_count_{3}, 
     process_pool_{process_data_thread_count_}
{
    init_user();

    init_topic_list();
}

KafkaQuote::KafkaQuote():
    process_data_thread_count_{3}, 
    process_pool_{process_data_thread_count_}
{
    this->bootstrap_servers_ = KAFKA_CONFIG.bootstrap_servers;

    init_user();

    init_topic_list();
}

KafkaQuote::~KafkaQuote()
{
    process_pool_.block();
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
        topic_list_.push_back("kline");
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

void KafkaQuote::start_listen_data()
{
    try
    {
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
        std::vector<std::string> src_data_vec;

        while (true) 
        {            
            auto records = consumer_sptr_->poll(std::chrono::milliseconds(100));
            std::cout << "records: " << records.size() << std::endl;

           
            for (const auto& record: records) 
            {                
                if (!record.error()) 
                {
                    string ori_data = record.value().toString();

                    LOG_INFO(ori_data);

                    // {
                    //     std::lock_guard<std::mutex> lk(src_data_mutex_);
                    //     src_data_vec_.emplace_back(std::move(ori_data));

                    // }

                    src_data_vec.emplace_back(std::move(ori_data));

                    // std::cout << "% Got a new message..." << std::endl;
                    // std::cout << "    Topic    : " << record.topic() << std::endl;
                    // std::cout << "    Partition: " << record.partition() << std::endl;
                    // std::cout << "    Offset   : " << record.offset() << std::endl;
                    // std::cout << "    Timestamp: " << record.timestamp().toString() << std::endl;
                    // std::cout << "    Headers  : " << kafka::toString(record.headers()) << std::endl;
                    // std::cout << "    Key   [" << record.key().toString() << "]" << std::endl;
                    // std::cout << "    Value [" << record.value().toString() << "]" << std::endl;
                } 
                else 
                {
                    LOG_WARN(record.toString());
                }
            }

            if (src_data_vec.size() > 0)
            {
                process_pool_.get_io_service().post(std::bind(&KafkaQuote::process_data, this, src_data_vec));
                src_data_vec.clear();
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
        process_pool_.start();

        // process_thread_ = std::thread(&KafkaQuote::process_data_main, this);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}

// void KafkaQuote::process_data_main() 
// {
//     try
//     {
//         process_pool_.start();

//         while(true)
//         {
//             std::unique_lock<std::mutex> lk(src_data_mutex_);
//             src_data_cv_.wait(lk, [&](){
//                 return !(src_data_vec_.size()==0);
//             });            

//             std::vector<std::string> cur_src_data_vec;
//             cur_src_data_vec.swap(src_data_vec_);


//         }

//         process_pool_.block();
//     }
//     catch(const std::exception& e)
//     {
//         LOG_ERROR(e.what());
//     }
// }

void KafkaQuote::process_data(std::vector<string> src_data_vec) 
{
    try
    {
        /* code */
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }
}
