#pragma once

#include <cstdint>
#include <cmath>

namespace Utils {

    class FractionalGenerator {
    private:
        double multiplier;
        double accumulator = 0.0;

    public:
        explicit FractionalGenerator(double mult) : multiplier(mult) {}

        uint64_t getFramesToGenerate() {
            accumulator += (multiplier - 1.0);
            const uint64_t frames = static_cast<uint64_t>(accumulator);
            accumulator -= frames;
            return frames;
        }

        void reset() { accumulator = 0.0; }
        
        static uint64_t getMaxGenerationCount(double mult) {
            return static_cast<uint64_t>(std::ceil(mult)) - 1;
        }
    };

}
