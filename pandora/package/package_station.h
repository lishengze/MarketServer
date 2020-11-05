#pragma once
#include "../pandora_declare.h"
#include "package.h"

// 数据包站点，数据包总是从一个站点流转到另一个站点，然后最终流出到其它的服务模块
class IPackageStation;
class IPackageStation
{
public:
    IPackageStation(IPackageStation* next_station) : next_station_{next_station}
    {
        if (next_station_)  next_station_->set_fore_station(this);
    }
    virtual ~IPackageStation() {}
    // 启动站点运营
    virtual void launch() {}
    // 释放运营站点
    virtual void release() {}
    // 设定下一个站点
    void set_fore_station(IPackageStation* fore_station) { fore_station_=fore_station; }
    // 所有的投递站点都需要继承这两个接口
    virtual void request_message(PackagePtr package) {}
    virtual void response_message(PackagePtr package) {}
    
    // 投递请求数据包到下一站点
    void deliver_request(PackagePtr package) { if(next_station_ && package) next_station_->request_message(package); }
    // 投递回应数据包到下一站点
    void deliver_response(PackagePtr package) {if(fore_station_ && package) fore_station_->response_message(package); }
protected:
    IPackageStation* fore_station_{nullptr};   // 上一站点，数据进来站点为初始站
    IPackageStation* next_station_{nullptr};   // 下一站点，数据出去站点为终点站
};
DECLARE_PTR(IPackageStation);