#pragma once

namespace utrade{namespace pandora{class UTLog;}}
class Package;
class CPackageDefineMap;
// UT debug the package
void UT_DEBUG_PACKAGE(Package* package, utrade::pandora::UTLog* logger);
void STG_DEBUG_PACKAGE(Package* package, utrade::pandora::UTLog* logger);

// UT check package
bool UT_CHECK_PACKAGE(Package* package);
bool STG_CHECK_PACKAGE(Package* package);
