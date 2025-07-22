#include "config/config.hpp"
#include "common/exception.hpp"

#include "config/default_conf.hpp"

#include <vulkan/vulkan_core.h>
#include <toml11/find.hpp>
#include <toml11/parser.hpp>
#include <toml.hpp>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <optional>
#include <fstream>
#include <cstdlib>
#include <utility>
#include <string>

using namespace Config;

namespace {
    Configuration globalConf{};
    std::optional<std::unordered_map<std::string, Configuration>> gameConfs;
}

Configuration Config::activeConf{};

namespace {
    /// Turn a string into a VkPresentModeKHR enum value.
    VkPresentModeKHR into_present(const std::string& mode) {
        if (mode == "fifo" || mode == "vsync")
            return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        if (mode == "mailbox")
            return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
        if (mode == "immediate")
            return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
        return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    }
}

void Config::updateConfig(const std::string& file) {
    if (!std::filesystem::exists(file)) {
        std::cerr << "lsfg-vk: Placing default configuration file at " << file << '\n';
        const auto parent = std::filesystem::path(file).parent_path();
        if (!std::filesystem::exists(parent))
            if (!std::filesystem::create_directories(parent))
                throw std::runtime_error("Unable to create configuration directory at " + parent.string());

        std::ofstream out(file);
        if (!out.is_open())
            throw std::runtime_error("Unable to create configuration file at " + file);
        out << DEFAULT_CONFIG;
        out.close();
    }

    // parse config file
    std::optional<toml::value> parsed;
    try {
        parsed.emplace(toml::parse(file));
        if (!parsed->contains("version"))
            throw std::runtime_error("Configuration file is missing 'version' field");
        if (parsed->at("version").as_integer() != 1)
            throw std::runtime_error("Configuration file version is not supported, expected 1");
    } catch (const std::exception& e) {
        throw LSFG::rethrowable_error("Unable to parse configuration file", e);
    }
    auto& toml = *parsed;

    // parse global configuration
    const toml::value globalTable = toml::find_or_default<toml::table>(toml, "global");
    const Configuration global{
        .dll =   toml::find_or(globalTable, "dll", std::string()),
        .config_file = file,
        .timestamp = std::filesystem::last_write_time(file)
    };

    // validate global configuration
    if (global.multiplier < 2)
        throw std::runtime_error("Global Multiplier cannot be less than 2");
    if (global.flowScale < 0.25F || global.flowScale > 1.0F)
        throw std::runtime_error("Flow scale must be between 0.25 and 1.0");

    // parse game-specific configuration
    std::unordered_map<std::string, Configuration> games;
    const toml::value gamesList = toml::find_or_default<toml::array>(toml, "game");
    for (const auto& gameTable : gamesList.as_array()) {
        if (!gameTable.is_table())
            throw std::runtime_error("Invalid game configuration entry");
        if (!gameTable.contains("exe"))
            throw std::runtime_error("Game override missing 'exe' field");

        const std::string exe = toml::find<std::string>(gameTable, "exe");
        Configuration game{
            .enable = true,
            .dll = global.dll,
            .multiplier = toml::find_or(gameTable, "multiplier", 2.0F),
            .flowScale = toml::find_or(gameTable, "flow_scale", 1.0F),
            .performance = toml::find_or(gameTable, "performance_mode", false),
            .hdr = toml::find_or(gameTable, "hdr_mode", false),
            .e_present =   into_present(toml::find_or(gameTable, "experimental_present_mode", "")),
            .config_file = file,
            .timestamp = global.timestamp
        };

        // validate the configuration
        if (game.multiplier < 1.0F)
            throw std::runtime_error("Multiplier cannot be less than 1.0");
        if (game.flowScale < 0.25F || game.flowScale > 1.0F)
            throw std::runtime_error("Flow scale must be between 0.25 and 1.0");
        games[exe] = std::move(game);
    }

    // store configurations
    globalConf = global;
    gameConfs = std::move(games);
}

Configuration Config::getConfig(const std::pair<std::string, std::string>& name) {
    // process legacy environment variables
    if (std::getenv("LSFG_LEGACY")) {
        Configuration conf{
            .enable = true,
            .multiplier = 2,
            .flowScale = 1.0F,
            .e_present = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR
        };

        const char* dll = std::getenv("LSFG_DLL_PATH");
        if (dll) conf.dll = std::string(dll);
        const char* multiplier = std::getenv("LSFG_MULTIPLIER");
        if (multiplier) conf.multiplier = std::stof(multiplier);
        const char* flow_scale = std::getenv("LSFG_FLOW_SCALE");
        if (flow_scale) conf.flowScale = std::stof(flow_scale);
        const char* performance = std::getenv("LSFG_PERFORMANCE_MODE");
        if (performance) conf.performance = std::string(performance) == "1";
        const char* hdr = std::getenv("LSFG_HDR_MODE");
        if (hdr) conf.hdr = std::string(hdr) == "1";
        const char* e_present = std::getenv("LSFG_EXPERIMENTAL_PRESENT_MODE");
        if (e_present) conf.e_present = into_present(std::string(e_present));

        return conf;
    }

    // process new configuration system
    if (!gameConfs.has_value())
        return globalConf;

    const auto& games = *gameConfs;
    auto it = std::ranges::find_if(games, [&name](const auto& pair) {
        return name.first.ends_with(pair.first) || (name.second == pair.first);
    });
    if (it != games.end())
        return it->second;

    return globalConf;
}

uint64_t Config::calculateGenerationCount(float multiplier) {
    // For true fractional multipliers, we need a more sophisticated approach.
    // The current frame generation system generates a fixed number of intermediate
    // frames between every pair of real frames.
    // 
    // For now, we'll use the ceiling of (multiplier - 1) to ensure we generate
    // enough frames to at least meet the target multiplier. A future implementation
    // could use temporal dithering or variable frame generation for exact fractional support.
    // 
    // Examples:
    // - multiplier 2.3 -> generate 2 frames (2.3 - 1 = 1.3, ceil = 2)
    // - multiplier 2.7 -> generate 2 frames (2.7 - 1 = 1.7, ceil = 2) 
    // - multiplier 3.1 -> generate 3 frames (3.1 - 1 = 2.1, ceil = 3)
    
    if (multiplier < 1.0F) {
        return 0; // No frame generation
    }
    
    // Calculate the number of intermediate frames needed
    float intermediateFrames = multiplier - 1.0F;
    
    // Use ceiling to ensure we meet or exceed the target
    // This will generate slightly more frames than requested for fractional values,
    // but provides a foundation for future exact fractional implementation
    return static_cast<uint64_t>(std::ceil(intermediateFrames));
}

uint64_t Config::calculateMaxGenerationCount(float multiplier) {
    // For variable frame generation, we need to allocate resources for the maximum
    // number of frames that might be generated in any single frame cycle.
    // This is the ceiling of (multiplier - 1).
    
    if (multiplier < 1.0F) {
        return 0;
    }
    
    float intermediateFrames = multiplier - 1.0F;
    return static_cast<uint64_t>(std::ceil(intermediateFrames));
}

uint64_t Config::calculateVariableGenerationCount(float multiplier, uint64_t frameIndex) {
    // For true fractional frame generation, we use temporal dithering.
    // This varies the number of generated frames over time to achieve 
    // the exact target multiplier on average.
    //
    // Example: multiplier 2.5 means we want 1.5 intermediate frames on average
    // - Sometimes generate 1 frame (for 2x total)
    // - Sometimes generate 2 frames (for 3x total)  
    // - Over time, average approaches 2.5x
    
    if (multiplier < 1.0F) {
        return 0; // No frame generation
    }
    
    float intermediateFrames = multiplier - 1.0F;
    uint64_t baseFrames = static_cast<uint64_t>(intermediateFrames); // Floor
    float fractionalPart = intermediateFrames - static_cast<float>(baseFrames);
    
    // Use temporal dithering based on frame index
    // This creates a repeating pattern that achieves the target average
    if (fractionalPart == 0.0F) {
        return baseFrames; // Perfect integer, no dithering needed
    }
    
    // Simple temporal dithering: every N frames, generate an extra frame
    // where N = 1/fractionalPart
    uint64_t period = static_cast<uint64_t>(1.0F / fractionalPart);
    bool generateExtra = (frameIndex % period) == 0;
    
    return baseFrames + (generateExtra ? 1 : 0);
}
}
