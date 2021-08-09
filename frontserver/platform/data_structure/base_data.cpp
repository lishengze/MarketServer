#include "base_data.h"

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
