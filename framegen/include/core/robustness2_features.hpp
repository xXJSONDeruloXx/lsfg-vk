#pragma once

#include <vulkan/vulkan_core.h>

namespace LSFG::Core {

    /// Features required by the v1 frame-generation compute device.
    ///
    /// Keep this construction in one place so the feature chain can be tested
    /// without creating a Vulkan device.
    [[nodiscard]] inline VkPhysicalDeviceRobustness2FeaturesEXT
    makeRobustness2Features() noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
            .robustImageAccess2 = VK_TRUE,
            .nullDescriptor = VK_TRUE,
        };
    }

}
