#include "base_data.h"
#include "../log/log.h"
#include <iostream>

WebsocketClassThreadSafe::~WebsocketClassThreadSafe()
{
    // std::cout << "delete " + get_ws_str() + ", id: " + std::to_string(id_) << std::endl;
    LOG_DEBUG("delete " + get_ws_str() + ", id: " + std::to_string(id_));
}


string get_comm_type_str(int type)
{
    string result = "UNKNOW_COMM_TYPE";

    switch (type)
    {
        case int(COMM_TYPE::HTTP):
            result = "HTTP";
            break;

        case int(COMM_TYPE::HTTPS):
            result = "HTTPS";
            break;

        case int(COMM_TYPE::WEBSOCKET):
            result = "WEBSOCKET";
            break;

        case int(COMM_TYPE::WEBSECKETS):
            result = "WEBSECKETS";
            break;                                    
    
    default:
        break;
    }

    return result;
}
