#pragma once
#include <random>

class RNG {
public:
	RNG();							 // Random seed
	explicit RNG(unsigned int seed); // Manual seed

	int NextInt(int min, int max);;
	float NextFloat(float min, float max); // Just 'cause
	bool Chance(float probability); // Returns true with a given probability e.g. 0.25f = 25% chance

private:
	std::mt19937 engine; // Mersenne Twister engine
};