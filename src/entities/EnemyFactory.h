// EnemyFactory.h
// ---------------
// Factory for creating enemies based on dungeon level.
// Selects from tiered templates and scales stats with level.

#pragma once
#include "Enemy.h"

class EnemyFactory {
public:
	// Create a random enemy scaled to the dungeon level
	static Enemy CreateEnemy(int dungeonLevel);
};
