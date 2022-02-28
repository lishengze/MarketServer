#include "risk_controller.h"
#include "pandora/util/time_util.h"
#include "Log/log.h"
#include "global_declare.h"
// #include "util/tool.h"

#include "risk_controller_config.h"
#include "pandora/util/path_util.h"

// exit handler function
void setup_signal_handler_callback()
{
    signal(SIGTERM, RiskControllerServer::signal_handler);
    signal(SIGINT, RiskControllerServer::signal_handler);
    signal(SIGHUP, RiskControllerServer::signal_handler);
    signal(SIGQUIT, RiskControllerServer::signal_handler);
    signal(SIGKILL, RiskControllerServer::signal_handler);
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


int main(int argc, char** argv) {

    string env = get_env(argc, argv);

    setup_signal_handler_callback();

    init_log(argv);

    string config_file_name = get_config_file_name(env);

    CONFIG->parse_config(config_file_name);

    RiskControllerServer riskControllerServer(config_file_name);
    riskControllerServer.start();
    
    utrade::pandora::io_service_pool engine_pool(3);
    // start pool
    engine_pool.start();
    // launch the engine
    engine_pool.block();

    std::cout << "exit" << std::endl;
    return 0;
}