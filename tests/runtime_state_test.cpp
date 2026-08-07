/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "instance.hpp"
#include "runtime_state.hpp"

#include "lsfg-vk-common/configuration/config.hpp"

#include <cmath>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <unistd.h>

namespace {
    class Environment {
    public:
        Environment(const char* name, const std::string& value) : name(name) {
            const char* old = std::getenv(name);
            if (old)
                this->old = std::string(old);
            setenv(name, value.c_str(), 1);
        }

        explicit Environment(const char* name) : name(name) {
            const char* old = std::getenv(name);
            if (old)
                this->old = std::string(old);
            unsetenv(name);
        }

        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        ~Environment() {
            if (this->old)
                setenv(this->name.c_str(), this->old->c_str(), 1);
            else
                unsetenv(this->name.c_str());
        }
    private:
        std::string name;
        std::optional<std::string> old;
    };

    class TemporaryConfig {
    public:
        TemporaryConfig() {
            this->path = std::filesystem::temp_directory_path()
                / ("lsfg-vk-runtime-test-" + std::to_string(getpid()));
            std::filesystem::remove_all(this->path);
            std::filesystem::create_directories(this->path);
            this->config_file = this->path / "conf.toml";
        }

        TemporaryConfig(const TemporaryConfig&) = delete;
        TemporaryConfig& operator=(const TemporaryConfig&) = delete;

        ~TemporaryConfig() {
            std::filesystem::remove_all(this->path);
        }

        [[nodiscard]] const auto& file() const { return this->config_file; }

        void write(std::optional<size_t> multiplier) {
            const auto previous = std::filesystem::exists(this->config_file)
                ? std::filesystem::last_write_time(this->config_file)
                : std::filesystem::file_time_type{};

            std::ofstream output(this->config_file);
            if (!output.is_open())
                throw std::runtime_error("unable to write temporary configuration");

            output << "version = 2\n\n[global]\nallow_fp16 = false\n";
            if (multiplier) {
                output << "\n[[profile]]\n"
                    << "name = 'Test profile'\n"
                    << "active_in = 'unused-test-process'\n"
                    << "multiplier = " << *multiplier << "\n"
                    << "flow_scale = 0.9\n"
                    << "performance_mode = false\n"
                    << "pacing = 'none'\n";
            }
            output.close();

            // Do not depend on filesystem timestamp granularity for the
            // configuration watcher.
            const auto current = std::filesystem::last_write_time(this->config_file);
            const auto minimum = previous + std::chrono::seconds(1);
            if (current <= previous)
                std::filesystem::last_write_time(this->config_file, minimum);
        }
    private:
        std::filesystem::path path;
        std::filesystem::path config_file;
    };

    void expect(bool condition, std::string_view message) {
        if (!condition)
            throw std::runtime_error(std::string(message));
    }

    void testPresentDecisionPermutations() {
        using lsfgvk::layer::PresentAction;
        using lsfgvk::layer::decidePresentAction;

        for (const bool compatible : { false, true }) {
            for (const bool contextReady : { false, true }) {
                expect(
                    decidePresentAction(false, compatible, contextReady)
                        == PresentAction::Passthrough,
                    "disabled FG must always pass through"
                );
            }
        }

        expect(
            decidePresentAction(true, true, true) == PresentAction::FrameGeneration,
            "an enabled compatible swapchain with a context must generate"
        );
        for (const auto [compatible, contextReady] : {
                std::pair{false, false},
                std::pair{false, true},
                std::pair{true, false}
            }) {
            expect(
                decidePresentAction(true, compatible, contextReady)
                    == PresentAction::RecreateSwapchain,
                "an enabled incomplete swapchain must request recreation"
            );
        }
    }

    void testMultiplierValidation() {
        TemporaryConfig files;
        Environment configPath("LSFGVK_CONFIG", files.file().string());
        Environment profile("LSFGVK_PROFILE", "Test profile");

        files.write(1);
        const ls::ConfigFile disabled(files.file());
        expect(disabled.profiles().front().multiplier == 1,
            "multiplier 1 must be accepted");
        expect(std::fabs(disabled.profiles().front().flow_scale - 0.9F) < 0.0001F,
            "decimal flow_scale must be parsed");

        files.write(0);
        bool rejected = false;
        try {
            const ls::ConfigFile invalid(files.file());
            (void) invalid;
        } catch (const std::exception&) {
            rejected = true;
        }
        expect(rejected, "multiplier 0 must remain invalid");
    }

    void testRootStateTransitions() {
        TemporaryConfig files;
        Environment configPath("LSFGVK_CONFIG", files.file().string());
        Environment profile("LSFGVK_PROFILE", "Test profile");

        files.write(1);
        lsfgvk::layer::Root root;
        expect(root.active(), "a matching 1x profile must attach the layer");
        expect(!root.frameGenerationEnabled(), "1x startup must be passthrough");
        expect(root.update(), "the initial watcher update must be observed");

        files.write(2);
        expect(root.update(), "1x to 2x must reload");
        expect(root.frameGenerationEnabled(), "2x must enable FG");

        files.write(1);
        expect(root.update(), "2x to 1x must reload");
        expect(!root.frameGenerationEnabled(), "2x to 1x must disable FG");

        files.write(std::nullopt);
        expect(root.update(), "profile removal must reload");
        expect(root.active(), "the layer must remain attached after profile removal");
        expect(!root.frameGenerationEnabled(), "profile removal must pass through");

        files.write(2);
        expect(root.update(), "profile re-addition must reload");
        expect(root.frameGenerationEnabled(), "profile re-addition must re-enable FG");

        files.write(0);
        bool rejected = false;
        try {
            (void) root.update();
        } catch (const std::exception&) {
            rejected = true;
        }
        expect(rejected, "an invalid live configuration must be rejected");
        expect(root.frameGenerationEnabled(),
            "a rejected live configuration must preserve the old state");
    }

    void testUnmatchedStartup() {
        TemporaryConfig files;
        Environment configPath("LSFGVK_CONFIG", files.file().string());
        Environment profile("LSFGVK_PROFILE", "No matching profile");

        files.write(2);
        const lsfgvk::layer::Root root;
        expect(!root.active(), "an unmatched app must not attach the layer");
        expect(!root.frameGenerationEnabled(), "an unmatched app must not generate");
    }

    void testEnvironmentMultiplierOne() {
        Environment envMode("LSFGVK_ENV", "1");
        Environment multiplier("LSFGVK_MULTIPLIER", "1");
        Environment profile("LSFGVK_PROFILE", "unused");

        const lsfgvk::layer::Root root;
        expect(root.active(), "environment configuration must attach the layer");
        expect(!root.frameGenerationEnabled(),
            "environment multiplier 1 must start in passthrough mode");
    }
}

int main() {
    try {
        testPresentDecisionPermutations();
        testMultiplierValidation();
        testRootStateTransitions();
        testUnmatchedStartup();
        testEnvironmentMultiplierOne();
    } catch (const std::exception& error) {
        std::cerr << "lsfg-vk runtime tests failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "lsfg-vk runtime tests passed\n";
    return 0;
}
