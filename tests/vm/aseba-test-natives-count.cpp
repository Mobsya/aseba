#include "vm/natives.h"
#include "common/consts.h"

// C++
#include <iostream>

static AsebaNativeFunctionPointer nativeFunctions[] = {
    ASEBA_NATIVES_STD_FUNCTIONS,
};

int main() {
    size_t nativesCount = (sizeof(nativeFunctions) / sizeof(AsebaNativeFunctionPointer));
    ASEBA_UNUSED(nativeFunctions);
    if(nativesCount != ASEBA_NATIVES_STD_COUNT)
        return 1;
    else
        return 0;
}
