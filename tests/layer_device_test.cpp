#include <volk.h>

#include <iostream>
#include <string_view>
#include <vector>

namespace {

constexpr int kSkipped = 77;

bool hasExtension(VkPhysicalDevice device, std::string_view name) {
    uint32_t count{};
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
        return false;

    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS)
        return false;

    for (const auto& extension : extensions) {
        if (std::string_view(extension.extensionName) == name)
            return true;
    }
    return false;
}

}

int main() {
    if (volkInitialize() != VK_SUCCESS)
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
    const VkResult instanceResult = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instanceResult == VK_ERROR_LAYER_NOT_PRESENT)
        return kSkipped;
    if (instanceResult != VK_SUCCESS)
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
    uint32_t selectedFamily{};
    for (const auto device : devices) {
        if (!hasExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            continue;

        uint32_t familyCount{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());
        for (uint32_t family = 0; family < familyCount; ++family) {
            if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                continue;
            selected = device;
            selectedFamily = family;
            break;
        }
        if (selected != VK_NULL_HANDLE)
            break;
    }

    if (selected == VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        return kSkipped;
    }

    constexpr float priority{1.0F};
    const VkDeviceQueueCreateInfo queueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = selectedFamily,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const VkDeviceCreateInfo deviceInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions,
    };

    VkDevice device{};
    const VkResult deviceResult = vkCreateDevice(selected, &deviceInfo, nullptr, &device);
    if (deviceResult != VK_SUCCESS) {
        std::cerr << "vkCreateDevice failed through the layer: " << deviceResult << '\n';
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
