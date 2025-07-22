#include "utils/fractional.hpp"
#include <iostream>
#include <iomanip>

int main() {
    // Test case 1: 1.3x multiplier should give exactly 30% more frames
    Utils::FractionalGenerator gen13(1.3);
    uint64_t total13 = 0;
    for (int i = 0; i < 10; i++) {
        auto frames = gen13.getFramesToGenerate();
        total13 += frames;
        std::cout << "1.3x iteration " << i << ": " << frames << " frames\n";
    }
    std::cout << "1.3x: " << total13 << " frames in 10 iterations (expected: 3)\n\n";
    
    // Test case 2: 2.5x multiplier
    Utils::FractionalGenerator gen25(2.5);
    uint64_t total25 = 0;
    for (int i = 0; i < 10; i++) {
        auto frames = gen25.getFramesToGenerate();
        total25 += frames;
        std::cout << "2.5x iteration " << i << ": " << frames << " frames\n";
    }
    std::cout << "2.5x: " << total25 << " frames in 10 iterations (expected: 15)\n\n";
    
    // Test max generation count calculation
    std::cout << "Max generation counts:\n";
    std::cout << "1.3x: " << Utils::FractionalGenerator::getMaxGenerationCount(1.3) << "\n";
    std::cout << "2.5x: " << Utils::FractionalGenerator::getMaxGenerationCount(2.5) << "\n";
    std::cout << "3.0x: " << Utils::FractionalGenerator::getMaxGenerationCount(3.0) << "\n";
    
    return 0;
}
