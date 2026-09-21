#include "support/AbiTest.h"


class CMyClass {

private:
    int x;
    int y;
    int z;

    BEGIN_FIXTURE()
        ENSURE_SIZE(12)
        ENSURE_OFFSET(x, 0)
        ENSURE_OFFSET(y, 4)
        ENSURE_OFFSET(z, 8)
    END_FIXTURE()

    RUN_FIXTURE(CMyClass)
};
