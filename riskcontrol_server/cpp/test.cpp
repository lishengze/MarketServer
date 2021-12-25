#include "test.h"
#include "Log/log.h"

#include "native_config.h"

TestEngine::~TestEngine()
{
    if (p_depth_riskctrl_)
    {
        delete p_depth_riskctrl_;
        p_depth_riskctrl_ = nullptr;
    }   
}

void TestEngine::init()
{
    try
    {
        p_depth_riskctrl_ = new DepthRiskCtrl();

        p_comm_server_ = new bcts::comm::Comm(server_address_, p_depth_riskctrl_, nullptr, nullptr);
        
        p_depth_riskctrl_->set_comm(p_comm_server_);

        nacos_config_.set_callback(this);

        LOG_INFO(string("Nacos Address: ") + NATIVE_CONFIG->nacos_addr_ + " namespace: " + NATIVE_CONFIG->nacos_namespace_);

        nacos_config_.start(NATIVE_CONFIG->nacos_addr_, NATIVE_CONFIG->nacos_namespace_);        
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }  
}

void TestEngine::start()
{
    try
    {
        p_comm_server_->launch();

        p_depth_riskctrl_->start();

        account_updater_.start(NATIVE_CONFIG->grpc_account_addr_, p_depth_riskctrl_);

        order_updater_.start(p_depth_riskctrl_);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void TestEngine::on_risk_config_update(const map<TSymbol, MarketRiskConfig>& config)
{
    try
    {
        p_depth_riskctrl_->on_risk_config_update(config);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }   
}

bool TestEngine::is_symbol_updated(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        bool result = false;
        for (auto iter:config)
        {
            if (meta_map_.find(iter.first) == meta_map_.end()) 
            {
                result = true;
                break;
            }
        }

        for (auto iter:meta_map_)
        {
            if (config.find(iter.first) == config.end()) 
            {
                result = true;
                break;
            }            
        }

        if (result)
        {
            std::map<TSymbol, std::set<TExchange>> new_meta;

            std::set<TExchange> exchange_set{MIX_EXCHANGE_NAME};

            for (auto iter:config)
            {
                new_meta[iter.first] = exchange_set;
            }
            meta_map_.swap(new_meta);
        }

        return result;
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }        
}

void TestEngine::on_symbol_config_update(const map<TSymbol, SymbolConfiguration>& config)
{
    try
    {
        if (is_symbol_updated(config)) 
        {
            p_comm_server_->set_depth_meta(meta_map_);
        }

        p_depth_riskctrl_->on_symbol_config_update(config);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }   
}

void TestEngine::on_hedge_config_update(const map<TSymbol, map<TExchange, HedgeConfig>>& config)
{
    try
    {
        p_depth_riskctrl_->on_hedge_config_update(config);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
    }    
}

void test_engine()
{
    string config_file_name = "config.json";
    NATIVE_CONFIG->parse_config(config_file_name);

    string server_address = "127.0.0.1:9117";
    TestEngine test_obj(server_address);
    test_obj.start();

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void TestMain()
{
    test_engine();
}