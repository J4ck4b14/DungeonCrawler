// EnemyFactory.h
// ---------------
// Factory for creating enemies based on dungeon level.
// Selects from tiered templates and scales stats with level.

#pragma once
#include "Enemy.h"

class RNG;
struct EnemyDefinition;

class EnemyFactory {
public:
	// Create a random enemy scaled to the dungeon level
	static Enemy CreateEnemy(int dungeonLevel);
	static Enemy CreateEnemy(int dungeonLevel, RNG& rng);
	static Enemy CreateEnemy(const EnemyDefinition& definition,
		int dungeonLevel, RNG& rng);
	static int CalculateRank(const EnemyDefinition& definition,
		int dungeonLevel, int specimenVariance);
};
