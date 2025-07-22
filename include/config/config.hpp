#pragma once

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <chrono>
#include <cstddef>
#include <string>

namespace Config {

    /// lsfg-vk configuration
    struct Configuration {
        /// Whether lsfg-vk should be loaded in the first place.
        bool enable{false};
        /// Path to Lossless.dll.
        std::string dll;

        /// The frame generation muliplier
        float multiplier{2.0F};
        /// The internal flow scale factor
        float flowScale{1.0F};
        /// Whether performance mode is enabled
        bool performance{false};
        /// Whether HDR is enabled
        bool hdr{false};

        /// Experimental flag for overriding the synchronization method.
        VkPresentModeKHR e_present;

        /// Path to the configuration file.
        std::filesystem::path config_file;
        /// File timestamp of the configuration file
        std::chrono::time_point<std::chrono::file_clock> timestamp;
    };

    /// Active configuration. Must be set in main.cpp.
    extern Configuration activeConf;

    ///
    /// Read the configuration file while preserving the previous configuration
    /// in case of an error.
    ///
    /// @param file The path to the configuration file.
    ///
    /// @throws std::runtime_error if an error occurs while loading the configuration file.
    ///
    void updateConfig(const std::string& file);

    ///
    /// Calculate the generation count (number of intermediate frames to generate)
    /// from a fractional multiplier value.
    ///
    /// @param multiplier The frame generation multiplier (e.g., 2.5 for 2.5x)
    /// @return The number of intermediate frames to generate
    ///
    uint64_t calculateGenerationCount(float multiplier);

    ///
    /// Calculate the maximum generation count needed for temporal dithering.
    /// This is used for resource allocation.
    ///
    /// @param multiplier The frame generation multiplier (e.g., 2.5 for 2.5x)
    /// @return The maximum number of intermediate frames that might be needed
    ///
    uint64_t calculateMaxGenerationCount(float multiplier);

    ///
    /// Calculate variable generation count for true fractional multipliers.
    /// This version takes a frame counter to enable temporal dithering.
    ///
    /// @param multiplier The frame generation multiplier (e.g., 2.5 for 2.5x)  
    /// @param frameIndex The current frame index for temporal variation
    /// @return The number of intermediate frames to generate for this frame
    ///
    uint64_t calculateVariableGenerationCount(float multiplier, uint64_t frameIndex);

    ///
    /// Get the configuration for a game.
    ///
    /// @param name The name of the executable to fetch.
    /// @return The configuration for the game or global configuration.
    ///
    /// @throws std::runtime_error if the configuration is invalid.
    ///
    Configuration getConfig(const std::pair<std::string, std::string>& name);

}
