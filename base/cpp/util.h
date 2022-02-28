#include "basic.h"

#include "base_data_stuct.h"

#include "pandora/util/path_util.h"

inline bool is_kline_valid(const KlineData& kline) 
{
    try
    {
        return !(kline.index < 1000000000 || kline.index > 1900000000 ||
                 kline.px_open<=0 || kline.px_close<=0 || kline.px_high<=0 || kline.px_low<=0);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return false;
}

inline bool is_trade_valid(const TradeData& trade)
{
    try
    {
        return trade.time > 1000000000000000000 && trade.time < 1900000000000000000;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return false;    
}

inline string get_env(int argc, char** argv)
{
    try
    {
        if(argc != 2)
        {
            cout << "================Invalid Usage!=================" << endl;
            cout << "====================Usage======================" << endl;
            cout << "./opu dev" << endl;
            cout << "./opu qa" << endl;
            cout << "./opu prd" << endl;
            cout << "./opu stg" << endl;
            cout << "=============================== end =====================" << endl;
            exit(0);
        }

        string env = argv[1];
        
        cout<<env<<endl;

        return env;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return "dev";
}

inline string get_config_file_name (string env)
{
    string result = "config.json";
    try
    {
        if ("prd" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/prd/" + "config.json";
        } 
        else if ("qa" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/qa/" + "config.json";
        } 
        else if ("stg" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/stg/" + "config.json";
        } 
        else if ("dev" == env)
        {
            result = utrade::pandora::get_module_path() +  "/etc/dev/" + "config.json";
        }
        else
        {
            result = utrade::pandora::get_module_path() +  "/etc/dev/" + "config.json";    
        }

        cout<<"result Path:" << result <<endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}