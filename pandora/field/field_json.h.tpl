/////////////////////////////////////////////////////////////////////////
///@depend envGenerated/UTType.xml
///@system UTrade交易系统
///@company 上海万向区块链股份公司
///@file UtPackageDesc.cpp
///@brief 定义了交易系统内部数据的底层支持类
///@history
///20190716      创建该文件
/////////////////////////////////////////////////////////////////////////
#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include "quark/cxx/ut/UtData.h"
#include "../util/json.hpp"

PANDORA_NAMESPACE_START
!!enter UT!!

inline std::string UTData2Jason(const void* data, short msg_type)
{
    nlohmann::json jsonFields;
    switch(msg_type)
    {
        !!travel fields!!
        !!let filed_name=@name!!
        case UT_FID_!!@name!!:
        {
            auto p!!@filed_name!!Field = (CUT!!@filed_name!!Field*)data;
            !!travel self!!
            assign(jsonFields["!!@name!!"], p!!@filed_name!!Field->!!@name!!);
            !!next!!
            return jsonFields.dump();
        }
        break;
        !!next!!
        default:
        {
            std::stringstream sstream;
            sstream << "Unexcept Type: " << msg_type;
            return sstream.str();
        }
    }
}

inline void Jason2UTData(void* data, short msg_type, std::string content)
{
    nlohmann::json js = nlohmann::json::parse(content);
    switch(msg_type)
    {
        !!travel fields!!
        !!let filed_name=@name!!
        case UT_FID_!!@name!!:
        {
            auto p!!@filed_name!!Field = (CUT!!@filed_name!!Field*)data;
            !!travel self!!
            !!if !strcmp(@cpp_type, "std::string")!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<std::string>().c_str());
            !!elseif !strcmp(@cpp_type, "double")!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<double>());
            !!elseif !strcmp(@cpp_type, "enum")!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<char>());
            !!elseif !strcmp(@cpp_type, "long")!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<long>());
            !!elseif !strcmp(@cpp_type, "int")!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<int>());
            !!else!!
            assign(p!!@filed_name!!Field->!!@name!!, js["!!@name!!"].get<unknown>());
            !!endif!!
            !!next!!
        }
        break;
        !!next!!
        default:
        {
            std::stringstream sstream;
            sstream << "Unexcept Type: " << msg_type;
        }
    }
}

!!leave!!

PANDORA_NAMESPACE_END



