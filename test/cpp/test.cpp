#include "test.h"
#include "global_declare.h"
#include "base/cpp/decimal.h"

void test_decimal()
{
    SDecimal oridata;
    oridata.from(0);

    if (oridata <= 0)
    {
        cout << "oridata <= 0" << endl;
    }
    else
    {
        cout << "Error " << endl;
    }
}

void TestMain()
{
    test_decimal();
}