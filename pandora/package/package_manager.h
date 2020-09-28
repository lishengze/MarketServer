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
    // exit instance
    void ExitInstance()
    {
        terminate = true;
        PackagePtr item;
        while (RspInfo_Queue_.try_dequeue(item)) item.reset();
        while (ReqCreateOrder_Queue_.try_dequeue(item)) item.reset();
        while (RspCreateOrder_Queue_.try_dequeue(item)) item.reset();
        while (RtnOrder_Queue_.try_dequeue(item)) item.reset();
        while (RtnTrade_Queue_.try_dequeue(item)) item.reset();
        while (ReqCancelOrder_Queue_.try_dequeue(item)) item.reset();
        while (RspCancelOrder_Queue_.try_dequeue(item)) item.reset();
        while (SubPosition_Queue_.try_dequeue(item)) item.reset();
        while (RtnPosition_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryOrder_Queue_.try_dequeue(item)) item.reset();
        while (RspQryOrder_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryTrade_Queue_.try_dequeue(item)) item.reset();
        while (RspQryTrade_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryAccount_Queue_.try_dequeue(item)) item.reset();
        while (RspQryAccount_Queue_.try_dequeue(item)) item.reset();
        while (SubAccount_Queue_.try_dequeue(item)) item.reset();
        while (RtnAccount_Queue_.try_dequeue(item)) item.reset();
        while (ReqLogin_Queue_.try_dequeue(item)) item.reset();
        while (RspLogin_Queue_.try_dequeue(item)) item.reset();
        while (ReqLogout_Queue_.try_dequeue(item)) item.reset();
        while (RspLogout_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryPosition_Queue_.try_dequeue(item)) item.reset();
        while (RspQryPosition_Queue_.try_dequeue(item)) item.reset();
        while (RtnPlatformDetail_Queue_.try_dequeue(item)) item.reset();
        while (RtnStrategyDetail_Queue_.try_dequeue(item)) item.reset();
        while (RtnDepth_Queue_.try_dequeue(item)) item.reset();
        while (RtnL2Depth_Queue_.try_dequeue(item)) item.reset();
        while (RtnL2Trade_Queue_.try_dequeue(item)) item.reset();
        while (RtnL2Order_Queue_.try_dequeue(item)) item.reset();
        while (RtnL2Index_Queue_.try_dequeue(item)) item.reset();
        while (RtnBarMarketData_Queue_.try_dequeue(item)) item.reset();
        while (RtnBusinessDebt_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryAccountBusiness_Queue_.try_dequeue(item)) item.reset();
        while (RspQryAccountBusiness_Queue_.try_dequeue(item)) item.reset();
        while (RtnAccountBusiness_Queue_.try_dequeue(item)) item.reset();
        while (ReqManualTransact_Queue_.try_dequeue(item)) item.reset();
        while (ReqTransact_Queue_.try_dequeue(item)) item.reset();
        while (RspTransact_Queue_.try_dequeue(item)) item.reset();
        while (RtnTransact_Queue_.try_dequeue(item)) item.reset();
        while (ReqQryTransact_Queue_.try_dequeue(item)) item.reset();
        while (RspQryTransact_Queue_.try_dequeue(item)) item.reset();

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

    /// 分配错误应答数据包
    PackagePtr AllocateRspInfo()
    {
        PackagePtr package;
        if (RspInfo_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspInfo(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求下单数据包
    PackagePtr AllocateReqCreateOrder()
    {
        PackagePtr package;
        if (ReqCreateOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqCreateOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqCreateOrderField);
        }
        return package;
    }
    /// 分配请求下单返回数据包
    PackagePtr AllocateRspCreateOrder()
    {
        PackagePtr package;
        if (RspCreateOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspCreateOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspCreateOrderField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配报单通知数据包
    PackagePtr AllocateRtnOrder()
    {
        PackagePtr package;
        if (RtnOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnOrderField);
        }
        return package;
    }
    /// 分配成交通知数据包
    PackagePtr AllocateRtnTrade()
    {
        PackagePtr package;
        if (RtnTrade_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnTrade(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnTradeField);
        }
        return package;
    }
    /// 分配请求撤单数据包
    PackagePtr AllocateReqCancelOrder()
    {
        PackagePtr package;
        if (ReqCancelOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqCancelOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqCancelOrderField);
        }
        return package;
    }
    /// 分配请求撤单返回数据包
    PackagePtr AllocateRspCancelOrder()
    {
        PackagePtr package;
        if (RspCancelOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspCancelOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspCancelOrderField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求订阅持仓信息数据包
    PackagePtr AllocateSubPosition()
    {
        PackagePtr package;
        if (SubPosition_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleSubPosition(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTSubPositionField);
        }
        return package;
    }
    /// 分配持仓信息通知数据包
    PackagePtr AllocateRtnPosition()
    {
        PackagePtr package;
        if (RtnPosition_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnPosition(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnPositionField);
        }
        return package;
    }
    /// 分配请求查询报单数据包
    PackagePtr AllocateReqQryOrder()
    {
        PackagePtr package;
        if (ReqQryOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryOrderField);
        }
        return package;
    }
    /// 分配返回请求查询报单数据包
    PackagePtr AllocateRspQryOrder()
    {
        PackagePtr package;
        if (RspQryOrder_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryOrder(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryOrderField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求查询成交数据包
    PackagePtr AllocateReqQryTrade()
    {
        PackagePtr package;
        if (ReqQryTrade_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryTrade(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryTradeField);
        }
        return package;
    }
    /// 分配返回请求查询成交数据包
    PackagePtr AllocateRspQryTrade()
    {
        PackagePtr package;
        if (RspQryTrade_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryTrade(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryTradeField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求查询账户数据包
    PackagePtr AllocateReqQryAccount()
    {
        PackagePtr package;
        if (ReqQryAccount_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryAccount(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryAccountField);
        }
        return package;
    }
    /// 分配返回请求查询账户数据包
    PackagePtr AllocateRspQryAccount()
    {
        PackagePtr package;
        if (RspQryAccount_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryAccount(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryAccountField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求订阅账户信息数据包
    PackagePtr AllocateSubAccount()
    {
        PackagePtr package;
        if (SubAccount_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleSubAccount(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTSubAccountField);
        }
        return package;
    }
    /// 分配账户信息通知数据包
    PackagePtr AllocateRtnAccount()
    {
        PackagePtr package;
        if (RtnAccount_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnAccount(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnAccountField);
        }
        return package;
    }
    /// 分配请求登录数据包
    PackagePtr AllocateReqLogin()
    {
        PackagePtr package;
        if (ReqLogin_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqLogin(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqLoginField);
        }
        return package;
    }
    /// 分配返回请求登录数据包
    PackagePtr AllocateRspLogin()
    {
        PackagePtr package;
        if (RspLogin_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspLogin(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspLoginField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求登出数据包
    PackagePtr AllocateReqLogout()
    {
        PackagePtr package;
        if (ReqLogout_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqLogout(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqLogoutField);
        }
        return package;
    }
    /// 分配返回请求登出数据包
    PackagePtr AllocateRspLogout()
    {
        PackagePtr package;
        if (RspLogout_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspLogout(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspLogoutField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配请求登出数据包
    PackagePtr AllocateReqQryPosition()
    {
        PackagePtr package;
        if (ReqQryPosition_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryPosition(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryPositionField);
        }
        return package;
    }
    /// 分配返回请求登出数据包
    PackagePtr AllocateRspQryPosition()
    {
        PackagePtr package;
        if (RspQryPosition_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryPosition(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryPositionField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配平台状态数据包
    PackagePtr AllocateRtnPlatformDetail()
    {
        PackagePtr package;
        if (RtnPlatformDetail_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnPlatformDetail(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnPlatformDetailField);
        }
        return package;
    }
    /// 分配策略状态通知数据包
    PackagePtr AllocateRtnStrategyDetail()
    {
        PackagePtr package;
        if (RtnStrategyDetail_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnStrategyDetail(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnStrategyDetailField);
        }
        return package;
    }
    /// 分配深度行情通知数据包
    PackagePtr AllocateRtnDepth()
    {
        PackagePtr package;
        if (RtnDepth_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnDepth(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnDepthField);
        }
        return package;
    }
    /// 分配L2深度行情通知数据包
    PackagePtr AllocateRtnL2Depth()
    {
        PackagePtr package;
        if (RtnL2Depth_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Depth(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnL2DepthField);
        }
        return package;
    }
    /// 分配L2成交通知数据包
    PackagePtr AllocateRtnL2Trade()
    {
        PackagePtr package;
        if (RtnL2Trade_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Trade(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnL2TradeField);
        }
        return package;
    }
    /// 分配L2报单通知数据包
    PackagePtr AllocateRtnL2Order()
    {
        PackagePtr package;
        if (RtnL2Order_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Order(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnL2OrderField);
        }
        return package;
    }
    /// 分配L2指数通知数据包
    PackagePtr AllocateRtnL2Index()
    {
        PackagePtr package;
        if (RtnL2Index_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Index(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnL2IndexField);
        }
        return package;
    }
    /// 分配K线通知数据包
    PackagePtr AllocateRtnBarMarketData()
    {
        PackagePtr package;
        if (RtnBarMarketData_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnBarMarketData(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnBarMarketDataField);
        }
        return package;
    }
    /// 分配Apollo子业务借贷关系数据包
    PackagePtr AllocateRtnBusinessDebt()
    {
        PackagePtr package;
        if (RtnBusinessDebt_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnBusinessDebt(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnBusinessDebtField);
        }
        return package;
    }
    /// 分配仅用于Apollo,查询账户资金数据包
    PackagePtr AllocateReqQryAccountBusiness()
    {
        PackagePtr package;
        if (ReqQryAccountBusiness_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryAccountBusiness(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryAccountBusinessField);
        }
        return package;
    }
    /// 分配仅用于Apollo,返回请求查询账户数据包
    PackagePtr AllocateRspQryAccountBusiness()
    {
        PackagePtr package;
        if (RspQryAccountBusiness_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryAccountBusiness(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryAccountBusinessField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配仅用于Apollo,账户信息通知数据包
    PackagePtr AllocateRtnAccountBusiness()
    {
        PackagePtr package;
        if (RtnAccountBusiness_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnAccountBusiness(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnAccountBusinessField);
        }
        return package;
    }
    /// 分配充值接口数据包
    PackagePtr AllocateReqManualTransact()
    {
        PackagePtr package;
        if (ReqManualTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqManualTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqManualTransactField);
        }
        return package;
    }
    /// 分配转账数据包
    PackagePtr AllocateReqTransact()
    {
        PackagePtr package;
        if (ReqTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqTransactField);
        }
        return package;
    }
    /// 分配转账回报数据包
    PackagePtr AllocateRspTransact()
    {
        PackagePtr package;
        if (RspTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspTransactField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }
    /// 分配转账通知数据包
    PackagePtr AllocateRtnTransact()
    {
        PackagePtr package;
        if (RtnTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRtnTransactField);
        }
        return package;
    }
    /// 分配转账查询请求数据包
    PackagePtr AllocateReqQryTransact()
    {
        PackagePtr package;
        if (ReqQryTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTReqQryTransactField);
        }
        return package;
    }
    /// 分配转账查询响应数据包
    PackagePtr AllocateRspQryTransact()
    {
        PackagePtr package;
        if (RspQryTransact_Queue_.try_dequeue(package))
        {
            ;
        }
        else
        {
            // generate a new package
            boost::shared_ptr<PackageManager> manager = shared_from_this();
            package = PackagePtr{new Package{}, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryTransact(package); }};
            package->SetPackageID(++package_id_);
            std::cout << "Create Package " << package->PackageID() << std::endl;
            CREATE_FIELD(package, CUTRspQryTransactField);
            CREATE_FIELD(package, CUTRspInfoField);
        }
        return package;
    }

    /// 回收错误应答数据包
    void RecycleRspInfo(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspInfo(package); }};
        RspInfo_Queue_.enqueue(ptr);
    }
    /// 回收请求下单数据包
    void RecycleReqCreateOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqCreateOrder(package); }};
        ReqCreateOrder_Queue_.enqueue(ptr);
    }
    /// 回收请求下单返回数据包
    void RecycleRspCreateOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspCreateOrder(package); }};
        RspCreateOrder_Queue_.enqueue(ptr);
    }
    /// 回收报单通知数据包
    void RecycleRtnOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnOrder(package); }};
        RtnOrder_Queue_.enqueue(ptr);
    }
    /// 回收成交通知数据包
    void RecycleRtnTrade(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnTrade(package); }};
        RtnTrade_Queue_.enqueue(ptr);
    }
    /// 回收请求撤单数据包
    void RecycleReqCancelOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqCancelOrder(package); }};
        ReqCancelOrder_Queue_.enqueue(ptr);
    }
    /// 回收请求撤单返回数据包
    void RecycleRspCancelOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspCancelOrder(package); }};
        RspCancelOrder_Queue_.enqueue(ptr);
    }
    /// 回收请求订阅持仓信息数据包
    void RecycleSubPosition(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleSubPosition(package); }};
        SubPosition_Queue_.enqueue(ptr);
    }
    /// 回收持仓信息通知数据包
    void RecycleRtnPosition(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnPosition(package); }};
        RtnPosition_Queue_.enqueue(ptr);
    }
    /// 回收请求查询报单数据包
    void RecycleReqQryOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryOrder(package); }};
        ReqQryOrder_Queue_.enqueue(ptr);
    }
    /// 回收返回请求查询报单数据包
    void RecycleRspQryOrder(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryOrder(package); }};
        RspQryOrder_Queue_.enqueue(ptr);
    }
    /// 回收请求查询成交数据包
    void RecycleReqQryTrade(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryTrade(package); }};
        ReqQryTrade_Queue_.enqueue(ptr);
    }
    /// 回收返回请求查询成交数据包
    void RecycleRspQryTrade(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryTrade(package); }};
        RspQryTrade_Queue_.enqueue(ptr);
    }
    /// 回收请求查询账户数据包
    void RecycleReqQryAccount(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryAccount(package); }};
        ReqQryAccount_Queue_.enqueue(ptr);
    }
    /// 回收返回请求查询账户数据包
    void RecycleRspQryAccount(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryAccount(package); }};
        RspQryAccount_Queue_.enqueue(ptr);
    }
    /// 回收请求订阅账户信息数据包
    void RecycleSubAccount(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleSubAccount(package); }};
        SubAccount_Queue_.enqueue(ptr);
    }
    /// 回收账户信息通知数据包
    void RecycleRtnAccount(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnAccount(package); }};
        RtnAccount_Queue_.enqueue(ptr);
    }
    /// 回收请求登录数据包
    void RecycleReqLogin(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqLogin(package); }};
        ReqLogin_Queue_.enqueue(ptr);
    }
    /// 回收返回请求登录数据包
    void RecycleRspLogin(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspLogin(package); }};
        RspLogin_Queue_.enqueue(ptr);
    }
    /// 回收请求登出数据包
    void RecycleReqLogout(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqLogout(package); }};
        ReqLogout_Queue_.enqueue(ptr);
    }
    /// 回收返回请求登出数据包
    void RecycleRspLogout(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspLogout(package); }};
        RspLogout_Queue_.enqueue(ptr);
    }
    /// 回收请求登出数据包
    void RecycleReqQryPosition(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryPosition(package); }};
        ReqQryPosition_Queue_.enqueue(ptr);
    }
    /// 回收返回请求登出数据包
    void RecycleRspQryPosition(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryPosition(package); }};
        RspQryPosition_Queue_.enqueue(ptr);
    }
    /// 回收平台状态数据包
    void RecycleRtnPlatformDetail(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnPlatformDetail(package); }};
        RtnPlatformDetail_Queue_.enqueue(ptr);
    }
    /// 回收策略状态通知数据包
    void RecycleRtnStrategyDetail(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnStrategyDetail(package); }};
        RtnStrategyDetail_Queue_.enqueue(ptr);
    }
    /// 回收深度行情通知数据包
    void RecycleRtnDepth(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnDepth(package); }};
        RtnDepth_Queue_.enqueue(ptr);
    }
    /// 回收L2深度行情通知数据包
    void RecycleRtnL2Depth(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Depth(package); }};
        RtnL2Depth_Queue_.enqueue(ptr);
    }
    /// 回收L2成交通知数据包
    void RecycleRtnL2Trade(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Trade(package); }};
        RtnL2Trade_Queue_.enqueue(ptr);
    }
    /// 回收L2报单通知数据包
    void RecycleRtnL2Order(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Order(package); }};
        RtnL2Order_Queue_.enqueue(ptr);
    }
    /// 回收L2指数通知数据包
    void RecycleRtnL2Index(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnL2Index(package); }};
        RtnL2Index_Queue_.enqueue(ptr);
    }
    /// 回收K线通知数据包
    void RecycleRtnBarMarketData(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnBarMarketData(package); }};
        RtnBarMarketData_Queue_.enqueue(ptr);
    }
    /// 回收Apollo子业务借贷关系数据包
    void RecycleRtnBusinessDebt(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnBusinessDebt(package); }};
        RtnBusinessDebt_Queue_.enqueue(ptr);
    }
    /// 回收仅用于Apollo,查询账户资金数据包
    void RecycleReqQryAccountBusiness(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryAccountBusiness(package); }};
        ReqQryAccountBusiness_Queue_.enqueue(ptr);
    }
    /// 回收仅用于Apollo,返回请求查询账户数据包
    void RecycleRspQryAccountBusiness(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryAccountBusiness(package); }};
        RspQryAccountBusiness_Queue_.enqueue(ptr);
    }
    /// 回收仅用于Apollo,账户信息通知数据包
    void RecycleRtnAccountBusiness(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnAccountBusiness(package); }};
        RtnAccountBusiness_Queue_.enqueue(ptr);
    }
    /// 回收充值接口数据包
    void RecycleReqManualTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqManualTransact(package); }};
        ReqManualTransact_Queue_.enqueue(ptr);
    }
    /// 回收转账数据包
    void RecycleReqTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqTransact(package); }};
        ReqTransact_Queue_.enqueue(ptr);
    }
    /// 回收转账回报数据包
    void RecycleRspTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspTransact(package); }};
        RspTransact_Queue_.enqueue(ptr);
    }
    /// 回收转账通知数据包
    void RecycleRtnTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRtnTransact(package); }};
        RtnTransact_Queue_.enqueue(ptr);
    }
    /// 回收转账查询请求数据包
    void RecycleReqQryTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleReqQryTransact(package); }};
        ReqQryTransact_Queue_.enqueue(ptr);
    }
    /// 回收转账查询响应数据包
    void RecycleRspQryTransact(Package* package)
    {
        std::cout << "Recycle Package " << package->PackageID() << std::endl;
        boost::shared_ptr<PackageManager> manager = shared_from_this();
        PackagePtr ptr{package, [manager](Package* package){ manager->terminate ? manager->RecyclePackage(package) : manager->RecycleRspQryTransact(package); }};
        RspQryTransact_Queue_.enqueue(ptr);
    }

    void RecyclePackage(Package* package)
    {
        std::cout << "Delete Package " << package->PackageID() << std::endl;
        package_ids_.erase(package->PackageID());
        delete package;
    }

private:
    // idle queue list
    /// 空闲错误应答数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspInfo_Queue_;
    /// 空闲请求下单数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqCreateOrder_Queue_;
    /// 空闲请求下单返回数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspCreateOrder_Queue_;
    /// 空闲报单通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnOrder_Queue_;
    /// 空闲成交通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnTrade_Queue_;
    /// 空闲请求撤单数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqCancelOrder_Queue_;
    /// 空闲请求撤单返回数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspCancelOrder_Queue_;
    /// 空闲请求订阅持仓信息数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> SubPosition_Queue_;
    /// 空闲持仓信息通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnPosition_Queue_;
    /// 空闲请求查询报单数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryOrder_Queue_;
    /// 空闲返回请求查询报单数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryOrder_Queue_;
    /// 空闲请求查询成交数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryTrade_Queue_;
    /// 空闲返回请求查询成交数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryTrade_Queue_;
    /// 空闲请求查询账户数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryAccount_Queue_;
    /// 空闲返回请求查询账户数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryAccount_Queue_;
    /// 空闲请求订阅账户信息数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> SubAccount_Queue_;
    /// 空闲账户信息通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnAccount_Queue_;
    /// 空闲请求登录数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqLogin_Queue_;
    /// 空闲返回请求登录数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspLogin_Queue_;
    /// 空闲请求登出数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqLogout_Queue_;
    /// 空闲返回请求登出数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspLogout_Queue_;
    /// 空闲请求登出数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryPosition_Queue_;
    /// 空闲返回请求登出数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryPosition_Queue_;
    /// 空闲平台状态数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnPlatformDetail_Queue_;
    /// 空闲策略状态通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnStrategyDetail_Queue_;
    /// 空闲深度行情通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnDepth_Queue_;
    /// 空闲L2深度行情通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnL2Depth_Queue_;
    /// 空闲L2成交通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnL2Trade_Queue_;
    /// 空闲L2报单通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnL2Order_Queue_;
    /// 空闲L2指数通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnL2Index_Queue_;
    /// 空闲K线通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnBarMarketData_Queue_;
    /// 空闲Apollo子业务借贷关系数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnBusinessDebt_Queue_;
    /// 空闲仅用于Apollo,查询账户资金数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryAccountBusiness_Queue_;
    /// 空闲仅用于Apollo,返回请求查询账户数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryAccountBusiness_Queue_;
    /// 空闲仅用于Apollo,账户信息通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnAccountBusiness_Queue_;
    /// 空闲充值接口数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqManualTransact_Queue_;
    /// 空闲转账数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqTransact_Queue_;
    /// 空闲转账回报数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspTransact_Queue_;
    /// 空闲转账通知数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RtnTransact_Queue_;
    /// 空闲转账查询请求数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> ReqQryTransact_Queue_;
    /// 空闲转账查询响应数据包队列
    moodycamel::ConcurrentQueue<PackagePtr> RspQryTransact_Queue_;

    // terminate the recycle
    bool terminate{false};
    // package id
    std::atomic<long> package_id_{0};
    // allocate package ids
    std::set<long> package_ids_;
};
DECLARE_PTR(PackageManager);

PANDORA_NAMESPACE_END
