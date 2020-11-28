#pragma once

#include "pandora/package/package.h"
#include "hub_struct.h"
#include "../front_server_declare.h"

void copy_sdepthdata(SDepthData* des, const SDepthData* src);

void copy_klinedata(KlineData* des, const KlineData* src);

PackagePtr GetNewSDepthDataPackage(const SDepthData& depth, int package_id);

PackagePtr GetNewKlineDataPackage(const KlineData& depth, int package_id);

string SDepthDataToJsonStr(const SDepthData& depth);

