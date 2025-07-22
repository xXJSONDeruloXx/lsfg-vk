#!/usr/bin/env python3
"""
Test script to demonstrate fractional frame generation logic
"""

def calculate_variable_generation_count(multiplier, frame_index):
    """Python version of our calculateVariableGenerationCount function"""
    if multiplier < 1.0:
        return 0
    
    intermediate_frames = multiplier - 1.0
    base_frames = int(intermediate_frames)  # Floor
    fractional_part = intermediate_frames - base_frames
    
    if fractional_part == 0.0:
        return base_frames  # Perfect integer
    
    # Temporal dithering
    period = int(1.0 / fractional_part)
    generate_extra = (frame_index % period) == 0
    
    return base_frames + (1 if generate_extra else 0)

def test_multiplier(multiplier, num_frames=20):
    """Test a multiplier over several frames"""
    print(f"\nTesting multiplier {multiplier}:")
    total_generated = 0
    
    for i in range(num_frames):
        gen_count = calculate_variable_generation_count(multiplier, i)
        total_frames = gen_count + 1  # +1 for the real frame
        total_generated += gen_count
        print(f"Frame {i:2d}: Generate {gen_count} -> Total {total_frames}x")
    
    # Calculate average multiplier achieved
    real_frames = num_frames
    total_output_frames = real_frames + total_generated
    actual_multiplier = total_output_frames / real_frames
    
    print(f"Target: {multiplier}x, Actual: {actual_multiplier:.3f}x, Error: {abs(actual_multiplier - multiplier):.3f}")

if __name__ == "__main__":
    test_multiplier(2.5, 10)
    test_multiplier(3.3, 15) 
    test_multiplier(2.0, 5)  # Perfect integer
