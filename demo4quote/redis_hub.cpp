#include "redis_hub.h"
#include "stream_engine.h"
#include "stream_engine_define.h"


void RedisHub::OnMessage(const std::string& channel, const std::string& msg){
    cout << "redis Receive Msg: " << channel << " Msg: " << msg << "\n" << endl;
    if (channel.find(DEPTH_UPDATE_HEAD)!=string::npos)
    {
        int pos = channel.find(DEPTH_UPDATE_HEAD);
        int pos2 = channel.find(".");
        string symbol = channel.substr(pos+strlen(DEPTH_UPDATE_HEAD), pos2-pos-strlen(DEPTH_UPDATE_HEAD));
        string exchange = channel.substr(pos2+1);
        engine_interface_->on_update(exchange, symbol);
    }
    else if(channel.find(TICK_HEAD)!=string::npos)
    {
    }
    else
    {
        //UT_LOG_WARNING(logger_, "Unknown Message Type");
    }
};