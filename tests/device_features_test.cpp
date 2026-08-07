#include "core/robustness2_features.hpp"

#include <cassert>

int main() {
    const auto features = LSFG::Core::makeRobustness2Features();

    assert(features.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT);
    assert(features.pNext == nullptr);
    assert(features.robustBufferAccess2 == VK_FALSE);
    assert(features.robustImageAccess2 == VK_TRUE);
    assert(features.nullDescriptor == VK_TRUE);
}
