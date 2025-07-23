#pragma once

#include <string>

const std::string DEFAULT_CONFIG = R"(version = 1
[global]
# override the location of Lossless Scaling
# dll = "/games/Lossless Scaling/Lossless.dll"

# GameScope frame pacing delay in microseconds for 2x multiplier compatibility
# Set to 4166 (~240fps pacing) to fix 2x frame generation display issues in Steam Deck Game Mode
# Only affects 2x multiplier when running under GameScope. Set to 0 to disable.
# gamescope_frame_delay = 4166

# [[game]] # example entry
# exe = "Game.exe"
#
# multiplier = 3
# flow_scale = 0.7
# performance_mode = true
# hdr_mode = false
#
# experimental_present_mode = "fifo"
# gamescope_frame_delay = 4166

[[game]] # default vkcube entry
exe = "vkcube"

multiplier = 4
performance_mode = true

[[game]] # default benchmark entry
exe = "benchmark"

multiplier = 4
performance_mode = false

[[game]] # override Genshin Impact
exe = "Genshin"

multiplier = 3
)";
