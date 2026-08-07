/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>

namespace lsfgvk::layer {

    /// Action to take for a present when the layer is attached to an application.
    enum class PresentAction : uint8_t {
        /// Pass the application's present through unchanged.
        Passthrough,
        /// Pass the present through, then request normal swapchain recreation.
        RecreateSwapchain,
        /// Run frame generation for the present.
        FrameGeneration
    };

    /// Decide how a swapchain can be presented under the current runtime state.
    ///
    /// A swapchain is compatible with frame generation only when it was created
    /// while the layer was enabled for frame generation. A missing context is
    /// treated the same way as an incompatible swapchain: presenting through it
    /// and returning VK_ERROR_OUT_OF_DATE_KHR lets the application recreate it
    /// through the normal Vulkan path.
    [[nodiscard]] constexpr PresentAction decidePresentAction(
            bool frameGenerationEnabled,
            bool swapchainCompatible,
            bool contextReady) noexcept {
        if (!frameGenerationEnabled)
            return PresentAction::Passthrough;

        if (!swapchainCompatible || !contextReady)
            return PresentAction::RecreateSwapchain;

        return PresentAction::FrameGeneration;
    }

}
