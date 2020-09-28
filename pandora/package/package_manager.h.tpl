/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UT.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190705  卞小兵    创建该文件
/////////////////////////////////////////////////////////////////////////
#pragma once

#include "../pandora_declare.h"
#include "../util/concurrentqueue.h"
#include "package.h"
#include <atomic>

PANDORA_NAMESPACE_START

class PackageManager : public boost::enable_shared_from_this<PackageManager>
{
public:
    PackageManager(){}
    virtual ~PackageManager()
    {
        std::cout << "Deconstruct PackageManager" << std::endl;
    }
    !!enter UT!!
    // exit instance
    void ExitInstance()
    {
        terminate = true;
        PackagePtr item;
        !!travel packages!!
        !!let package_name=@name!!
        while (!!@package_name!!_Queue_.try_dequeue(item)) item.reset();
        !!next!!

        while (!package_ids_.empty())
        {
            std::cout << "Wait For Recycle Packages: ";
            for (auto& id : package_ids_)
            {
                std::cout << id << " ";
            }
            sleep(1);
        }
    }

    !!travel packages!!
    !!let package_name=@name!!
    /// 分配!!@comment!!数据包
    PackagePtr Allocate!!@package_name!!()
    {
        PackagePtr package;
        if (!!@package_name!!_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->Recycle!!@package_name!!(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            !!if !strncmp(@package_name, "Rsp", 3) && strncmp(@package_name, "RspInfo", 7)!!
            CREATE_FIELD(package, CUT!!@package_name!!Field);
            CREATE_FIELD(package, CUTRspInfoField);
            !!else!!
            CREATE_FIELD(package, CUT!!@package_name!!Field);
            !!endif!!
        }
        return package;
    }
    !!next!!

    !!travel packages!!
    !!let package_name=@name!!
    /// 回收!!@comment!!数据包
    void Recycle!!@package_name!!(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->Recycle!!@package_name!!(package); }};
        !!@package_name!!_Queue_.enqueue(ptr);
    }
    !!next!!

    void RecyclePackage(Package* package)
    {
        std::cout << "Delete Package " << package->PackageID() << std::endl;
        package_ids_.erase(package->PackageID());
        delete package;
    }

private:
    // idle queue list
    !!travel packages!!
    !!let package_name=@name!!
    /// 空闲!!@comment!!数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> !!@package_name!!_Queue_;
    !!next!!

    !!leave!!
    // terminate the recycle
    bool terminate{false};
    // package id
    std::atomic<long> package_id_{0};
    // allocate package ids
    std::set<long> package_ids_;
};
DECLARE_PTR(PackageManager);

PANDORA_NAMESPACE_END