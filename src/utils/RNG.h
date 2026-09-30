// Thin RNG wrapper. Seeded construction is used by tests and validation reports;
// the default constructor uses platform entropy for ordinary play.

#pragma once
#include <random>

class RNG {
public:
	RNG();
	explicit RNG(unsigned int seed);

	int NextInt(int min, int max);
	float NextFloat(float min, float max);
	bool Chance(float probability);

private:
	std::mt19937 engine;
};
