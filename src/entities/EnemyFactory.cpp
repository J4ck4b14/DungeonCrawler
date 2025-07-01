#include "EnemyFactory.h"
#include "utils/RNG.h"
#include <vector>

Enemy EnemyFactory::CreateRandomEnemy() {
	static RNG rng; // One RNG per program - clean and testable

	// Define enemy types and their attributes
	struct EnemyType {
		std::string name;
		int minHP, maxHP;
		int minATK, maxATK;
	};

	const std::vector<EnemyType> enemyTypes = {
		{"Goblin", 10, 20, 2, 5},
        {"Bandit", 8, 18, 1, 4},
        {"Demon", 30, 60, 7, 15},
        {"Dragon", 50, 100, 10, 20},
        {"Ghost", 12, 22, 2, 5},
        {"Giant", 40, 80, 8, 16},
        {"Goblin", 10, 20, 2, 5},
        {"Orc", 15, 30, 3, 7},
        {"Skeleton", 8, 15, 1, 4},
        {"Slime", 3, 10, 1, 2},
        {"Spider", 5, 15, 1, 3},
        {"Troll", 20, 40, 4, 8},
        {"Vampire", 20, 40, 5, 10},
        {"Werewolf", 25, 50, 6, 12},
        {"Witch", 18, 35, 4, 8},
        {"Zombie", 12, 25, 2, 6}
	};
	
	const EnemyType& enemyType = enemyTypes[rng.NextInt(0, static_cast<int>(enemyTypes.size()) - 1)];

	int hp = rng.NextInt(enemyType.minHP, enemyType.maxHP);
	int atk = rng.NextInt(enemyType.minATK, enemyType.maxATK);

	return Enemy(enemyType.name, hp, atk);
}