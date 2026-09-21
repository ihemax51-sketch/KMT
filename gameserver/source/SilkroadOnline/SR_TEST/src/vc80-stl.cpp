#include "vc80-stl.h"

#include "Test/Test.h"

#include <atlstr.h>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <map>

bool vc80_vector();

bool vc80_stl() {
    return vc80_vector();
}

bool vc80_vector() {
    bool failed = false;
    failed |= check_size(std::vector<void *>, 0x10);
    failed |= check_size(std::list<void *>, 0xC);
    failed |= check_size(std::queue<int>, 0x14);
    failed |= check_size(std::set<int>, 0xc);
    failed |= check_size(std::string, 0xC);
    failed |= check_size(std::wstring, 0xC);
    failed |= check_size(long, 0x04);

    return failed;
}
