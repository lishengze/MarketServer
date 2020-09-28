#include "package.h"
#include "quark/cxx/stg/StgPackageDesc.h"
#include "quark/cxx/ut/UtPackageDesc.h"
#include "../messager/ut_log.h"
#include "quark/cxx/PackageDesc.h"
#include "quark/cxx/ut/UtPrintUtils.h"
#include "quark/cxx/stg/StgPrintUtils.h"


// DEBUG package
void DEBUG_PACKAGE(unsigned int tid, Package* package, utrade::pandora::UTLog* logger, bool ut_package=true)
{
    std::stringstream sstream;
    sstream << "PACKAGE CONTENT START" << std::endl;
    TPackageDefine* pPackageDefine=nullptr;
    if (ut_package)
        pPackageDefine = g_UTCPackageDefineMap.Find(tid);
    else
        pPackageDefine = g_STGCPackageDefineMap.Find(tid);
    if (!pPackageDefine)
    {
        UT_LOG_ERROR_FMT(logger, "Can't find package define [0x%08x]", tid);
        return;
    }

    // we can't do this config
    if (pPackageDefine->fieldUseCount < 1 || pPackageDefine->fieldUseCount > 2)
    {
        UT_LOG_ERROR_FMT(logger, "Package: [0x%08x] Field Use Count Error: %d", tid, pPackageDefine->fieldUseCount);
        return;
    }

    sstream << std::setw(22) << package->head_str() << std::endl;
    // loop all field in the package
    for (size_t pfidx = 0; pfidx != package->Fields.size(); ++pfidx)
    {
        TFieldUse *pFieldUse = pPackageDefine->fieldUse;
        int fidx = 0;
        for (fidx = 0; fidx != pPackageDefine->fieldUseCount; ++ fidx)
        {
            if (pFieldUse->fid == package->Fields[pfidx].FieldID)
            {
                if (ut_package)
                    sstream << convertUTData(package->Fields[pfidx].Field.get(), pFieldUse->fid).c_str();
                else
                    sstream << convertSTGData(package->Fields[pfidx].Field.get(), pFieldUse->fid).c_str();
                break;
            }
            pFieldUse++;
        }
        if (fidx == pPackageDefine->fieldUseCount)
        {
            UT_LOG_ERROR_FMT(logger, "Package: [0x%08x] Field ID Error: %ld", tid, package->Fields[pfidx].FieldID);
            break;
        }
    }
    sstream << std::setw(22) << " " << " PACKAGE CONTENT END";
    UT_LOG_DEBUG(logger, sstream.str().c_str());
}

// DEBUG package
void UT_DEBUG_PACKAGE(Package* package, utrade::pandora::UTLog* logger)
{
    DEBUG_PACKAGE(package->Tid(), package, logger);
}

void STG_DEBUG_PACKAGE(Package* package, utrade::pandora::UTLog* logger)
{
    DEBUG_PACKAGE(package->Tid(), package, logger, false);
}

// check package
bool CHECK_PACKAGE(unsigned int tid, Package* package, bool ut_package=true)
{
    TPackageDefine* pPackageDefine = nullptr;
    if (ut_package)
        pPackageDefine = g_UTCPackageDefineMap.Find(tid);
    else
        pPackageDefine = g_STGCPackageDefineMap.Find(tid);
    if (!pPackageDefine)
    {
        return false;
    }
    // fieldmap use statistic
    std::map<int, int> currFieldMap;
    // loop all fields
    for (size_t pfidx = 0; pfidx != package->Fields.size(); ++pfidx)
    {
        TFieldUse *pFieldUse = pPackageDefine->fieldUse;
        int fidx = 0;
        for (fidx = 0; fidx != pPackageDefine->fieldUseCount; ++fidx)
        {
            if (pFieldUse->fid == package->Fields[pfidx].FieldID)
            {
                currFieldMap[pFieldUse->fid]++;
                break;
            }
            pFieldUse++;
        }
        if (fidx == pPackageDefine->fieldUseCount)
        {
            return false;
        }
    }
    // check the field valid
    TFieldUse *pFieldUse = pPackageDefine->fieldUse;
    for (int j = 0; j < pPackageDefine->fieldUseCount; j++, pFieldUse++)
    {
        auto it = currFieldMap.find(pFieldUse->fid);
        if (it == currFieldMap.end())
        {
            if (pFieldUse->minOccur != 0)
            {
                return false;
            }
        }
        else
        {
            if (pFieldUse->minOccur > it->second)
            {
                return false;
            }
            if (pFieldUse->maxOccur < it->second)
            {
                return false;
            }
        }
    }
    return true;
}

// check package
bool UT_CHECK_PACKAGE(Package* package)
{
    return CHECK_PACKAGE(package->Tid(), package);
}

bool STG_CHECK_PACKAGE(Package* package)
{
    return CHECK_PACKAGE(package->Tid(), package, false);
}