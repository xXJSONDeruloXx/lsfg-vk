#include <volk.h>

#include "core/robustness2_features.hpp"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

constexpr int kSkipped = 77;

bool hasRobustness2Extension(VkPhysicalDevice device) {
    uint32_t count{};
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
        return false;

    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS)
        return false;

    for (const auto& extension : extensions) {
        if (std::string_view(extension.extensionName) == VK_EXT_ROBUSTNESS_2_EXTENSION_NAME)
            return true;
    }
    return false;
}

}

int main() {
    if (volkInitialize() != VK_SUCCESS)
        return kSkipped;
    if (volkGetInstanceVersion() < VK_API_VERSION_1_1)
        return kSkipped;

    const VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1,
    };
    const VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
    };

    VkInstance instance{};
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
        return kSkipped;
    volkLoadInstance(instance);

    uint32_t deviceCount{};
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0) {
        vkDestroyInstance(instance, nullptr);
        return kSkipped;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        return kSkipped;
    }

    VkPhysicalDevice selected{};
    VkPhysicalDeviceRobustness2FeaturesEXT supportedRobustness2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
    };
    for (const auto device : devices) {
        if (!hasRobustness2Extension(device))
            continue;

        VkPhysicalDeviceFeatures2 supportedFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &supportedRobustness2,
        };
        vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);
        if (supportedRobustness2.robustImageAccess2 == VK_TRUE &&
            supportedRobustness2.nullDescriptor == VK_TRUE) {
            selected = device;
            break;
        }
    }

    if (selected == VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        return kSkipped;
    }

    uint32_t familyCount{};
    vkGetPhysicalDeviceQueueFamilyProperties(selected, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(selected, &familyCount, families.data());

    uint32_t computeFamily{};
    bool foundCompute{};
    for (uint32_t i = 0; i < familyCount; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
            computeFamily = i;
            foundCompute = true;
            break;
        }
    }
    if (!foundCompute) {
        vkDestroyInstance(instance, nullptr);
        return kSkipped;
    }

    constexpr float priority{1.0F};
    const VkDeviceQueueCreateInfo queueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = computeFamily,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    auto requestedRobustness2 = LSFG::Core::makeRobustness2Features();
    const char* extensions[] = { VK_EXT_ROBUSTNESS_2_EXTENSION_NAME };
    const VkDeviceCreateInfo deviceInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &requestedRobustness2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
    };

    VkDevice device{};
    const VkResult result = vkCreateDevice(selected, &deviceInfo, nullptr, &device);
    if (result != VK_SUCCESS) {
        std::cerr << "vkCreateDevice rejected the requested robustness2 feature chain: "
                  << result << '\n';
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
