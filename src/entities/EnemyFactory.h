#pragma once
#include "Enemy.h"

class EnemyFactory {
public:
	// Create a random enemy scaled to the dungeon level
	static Enemy CreateEnemy(int dungeonLevel);
};
