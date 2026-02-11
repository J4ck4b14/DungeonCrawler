// RNG.h
// -----
// Random number generator utility wrapping the Mersenne Twister engine.
// Provides integer/float ranges and probability-based coin flips.
// Can be seeded manually for reproducibility or randomly via std::random_device.

#pragma once
#include <random>

class RNG {
public:
	RNG();                       // Random seed via std::random_device
	explicit RNG(unsigned int seed); // Manual seed for reproducibility

	int NextInt(int min, int max);       // Uniform random integer in [min, max]
	float NextFloat(float min, float max); // Uniform random float in [min, max]
	bool Chance(float probability);      // Returns true with given probability (0.0 - 1.0)

private:
	std::mt19937 engine;
};