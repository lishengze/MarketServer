#include "config.h"
#include <fstream>
#include "pandora/util/json.hpp"
#include "../log/log.h"

void Config::load_config(string file_name)
{
    try
    {    
        std::ifstream in_config(file_name);

        if (!in_config.is_open())
        {
            cout << "Failed to Open: " << file_name << endl;
        }
        else
        {
            string contents((istreambuf_iterator<char>(in_config)), istreambuf_iterator<char>());
            nlohmann::json js = nlohmann::json::parse(contents);
            
            if (!js["hub"].is_null() && !js["hub"]["risk_controller_addr"].is_null())
            {
                hub_address_ = js["hub"]["risk_controller_addr"].get<string>();
            }
            else
            {
                string error_msg = file_name + " does not have hub.addr! \n";
                // cout << error_msg << endl; 
                LOG_ERROR(error_msg);
            }

            if (!js["ws_server"].is_null() && !js["ws_server"]["port"].is_null())
            {
                ws_port_ = js["ws_server"]["port"].get<int>();                
            }
            else
            {
                string error_msg = file_name + " does not have ws_server.port! \n";
                LOG_ERROR(error_msg);                
            }

            if (!js["rest_server"].is_null() && !js["rest_server"]["port"].is_null())
            {
                rest_port_ = js["rest_server"]["port"].get<int>();
            }
            else
            {
                string error_msg = file_name + " does not have rest_server.port! \n";
                LOG_ERROR(error_msg); 
            }                        
        }
    
    }
    catch(const std::exception& e)
    {
        std::cerr << "Config::load_config: " << e.what() << '\n';
    }



    // key_center_info_.Addr = js["KeyCenter"]["Addr"].get<string>();
    // key_center_info_.Port = js["KeyCenter"]["Port"].get<int>();
    // key_center_info_.UserName = js["KeyCenter"]["UserName"].get<string>();
    // key_center_info_.Password = js["KeyCenter"]["Password"].get<string>();

    // debug_client_recv_package = bool(js["debug_client_recv_package"].get<int>());
    // debug_client_send_package = bool(js["debug_client_send_package"].get<int>());
    // debug_console_recv_package = bool(js["debug_console_recv_package"].get<int>());
    // debug_console_send_package = bool(js["debug_console_send_package"].get<int>());
    // debug_bridge_recv_package = bool(js["debug_bridge_recv_package"].get<int>());
    // debug_bridge_send_package = bool(js["debug_bridge_send_package"].get<int>());
}