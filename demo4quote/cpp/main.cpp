#include "stream_engine_config.h"
#include "stream_engine.h"
#include "kline_mixer_test.h"
#include "pandora/util/io_service_pool.h"
#include "report.h"

#include "Log/log.h"
#include "global_declare.h"
#include "pandora/util/path_util.h"

// config file relative path
const char* config_file = "config.json";

// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, StreamEngine::signal_handler);
    signal(SIGINT, StreamEngine::signal_handler);
    signal(SIGHUP, StreamEngine::signal_handler);
    signal(SIGQUIT, StreamEngine::signal_handler);
    signal(SIGKILL, StreamEngine::signal_handler);
}

void init_log(char** argv)
{
    string program_full_name = argv[0];

    string work_dir = utrade::pandora::get_work_dir_name (program_full_name);
    string program_name = utrade::pandora::get_program_name(program_full_name);

    cout << "program_full_name: " << program_full_name << "\n"
            << "work_dir: " << work_dir << "\n"
            << "program_name: " << program_name << "\n"
            << endl;

    LOG->set_work_dir(work_dir);
    LOG->set_program_name(program_name);
    LOG->start();   
}

inline string get_env(int argc, char** argv)
{
    try
    {
        if(argc != 2)
        {
            cout << "================Invalid Usage!=================" << endl;
            cout << "====================Usage======================" << endl;
            cout << "./opu -dev" << endl;
            cout << "./opu -qa" << endl;
            cout << "./opu -prd" << endl;
            cout << "./opu -stg" << endl;
            cout << "=============================== end =====================" << endl;
            exit(0);
        }

        string env = argv[1];
        
        cout<< "env: " << env<<endl;

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
        if ("-prd" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/prd/" + "config.json";
        } 
        else if ("-qa" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/qa/" + "config.json";
        } 
        else if ("-stg" == env) 
        {
            result = utrade::pandora::get_module_path() +  "/etc/stg/" + "config.json";
        } 
        else if ("-dev" == env)
        {
            result = utrade::pandora::get_module_path() +  "/etc/dev/" + "config.json";
        }
        else
        {
            result = utrade::pandora::get_module_path() +  "/etc/dev/" + "config.json";    
        }

        cout<<"result Path:" << result <<endl;

        return result;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }    
}


int main(int argc, char** argv) 
{
    string env = get_env(argc, argv);

    setup_signal_handler_callback();
    
    init_log(argv);

    string config_file_name = get_config_file_name(env);

    // if(argc == 2)
    // {
    //     config_file_name = argv[1];
    //     cout << "config_file_name: " << config_file_name << endl;
    // }

    utrade::pandora::Singleton<Config>::Instance();
    CONFIG->parse_config(config_file_name);

    // string program_full_name = argv[0];

    // string work_dir = utrade::pandora::get_work_dir_name (program_full_name);
    // string program_name = utrade::pandora::get_program_name(program_full_name);

    // cout << "program_full_name: " << program_full_name << "\n"
    //      << "work_dir: " << work_dir << "\n"
    //      << "program_name: " << program_name << "\n"
    //      << endl;

    // LOG->set_work_dir(work_dir);
    // LOG->set_program_name(program_name);
    // LOG->start();

    // // init configuration
    // utrade::pandora::Singleton<Config>::Instance();
    // CONFIG->parse_config(config_file_name);

    StreamEngine streamEngine;
    streamEngine.init();
    streamEngine.start();

    // Report report;
    // report.start();
    
    utrade::pandora::io_service_pool engine_pool(1);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}