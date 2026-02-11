// RNG.cpp
// -------
// Implementation of the RNG utility.

#include "RNG.h"

RNG::RNG() {
	std::random_device rd;
	engine = std::mt19937(rd()); // Random seed
}

RNG::RNG(unsigned int seed) {
	engine = std::mt19937(seed); // Manual seed
}

int RNG::NextInt(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(engine);
}

float RNG::NextFloat(float min, float max) {
	std::uniform_real_distribution<float> dist(min, max);
	return dist(engine);
}

bool RNG::Chance(float probability) {
	std::bernoulli_distribution dist(probability); 
	return dist(engine);
}