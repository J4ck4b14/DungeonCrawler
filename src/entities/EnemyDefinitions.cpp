#include "EnemyDefinitions.h"

const std::vector<EnemyDefinition>& GetEnemyDefinitions() {
	static const std::vector<EnemyDefinition> definitions = {
		// Base ranges define species identity. Rank growth is deliberately
		// per-stat and per-species rather than a shared all-stat multiplier.
		{EnemyArchetype::Slime, "Slime", 8,14, 1,3, 1,2, 0,0, 1,5,
			SpellElement::Fire, {}, {0, .70,.08,.02,.00,.00,.35}},
		{EnemyArchetype::Rat, "Rat", 6,10, 1,2, 3,4, 0,0, 1,4,
			SpellElement::Fire, {}, {0, .35,.05,.12,.00,.00,.30}},
		{EnemyArchetype::Skeleton, "Skeleton", 10,16, 2,3, 1,2, 0,0, 1,6,
			SpellElement::Arcane, {}, {1, .65,.10,.02,.00,.00,.40}},
		{EnemyArchetype::Spider, "Spider", 7,12, 1,3, 2,3, 0,0, 1,5,
			SpellElement::Fire, {}, {1, .45,.07,.08,.00,.00,.40}},
		{EnemyArchetype::Goblin, "Goblin", 10,16, 2,4, 2,3, 1,1, 1,8,
			SpellElement::Ice, {"Spark"}, {2, .55,.08,.08,.04,.12,.50}},

		{EnemyArchetype::Bandit, "Bandit", 14,22, 3,5, 2,3, 1,2, 3,12,
			SpellElement::Lightning, {"Frost Bolt"}, {2, .70,.10,.06,.04,.12,.65}},
		{EnemyArchetype::Orc, "Orc", 18,28, 4,6, 1,2, 0,1, 3,14,
			SpellElement::Fire, {}, {3, 1.10,.18,.015,.01,.00,.80}},
		{EnemyArchetype::Ghost, "Ghost", 12,18, 2,4, 3,4, 2,3, 3,15,
			SpellElement::Arcane, {"Shadow Bolt", "Frost Bolt"}, {3, .55,.04,.06,.10,.30,.85}},
		{EnemyArchetype::Witch, "Witch", 14,20, 2,3, 2,3, 3,4, 3,18,
			SpellElement::Shadow, {"Fireball", "Heal", "Shadow Bolt"}, {4, .55,.04,.04,.16,.45,1.00}},

		{EnemyArchetype::Troll, "Troll", 25,38, 5,7, 1,2, 0,1, 5,25,
			SpellElement::Fire, {}, {4, 1.50,.16,.015,.01,.00,1.25}},
		{EnemyArchetype::Werewolf, "Werewolf", 22,34, 5,8, 3,5, 0,1, 5,28,
			SpellElement::Ice, {}, {4, .90,.13,.16,.01,.00,1.35}},
		{EnemyArchetype::Vampire, "Vampire", 20,30, 4,6, 3,4, 2,4, 5,30,
			SpellElement::Fire, {"Shadow Bolt", "Heal", "Soul Drain"}, {5, .85,.09,.09,.11,.30,1.45}},
		{EnemyArchetype::DarkMage, "Dark Mage", 16,24, 2,4, 2,3, 4,5, 5,32,
			SpellElement::Shadow, {"Fireball", "Thunderbolt", "Ice Shard", "Heal"}, {5, .55,.03,.03,.20,.60,1.55}},

		{EnemyArchetype::Demon, "Demon", 35,50, 6,10, 2,4, 3,5, 8,50,
			SpellElement::Ice, {"Inferno", "Void Blast", "Soul Drain"}, {6, 1.10,.15,.05,.14,.45,2.10}},
		{EnemyArchetype::Giant, "Giant", 45,60, 7,11, 1,2, 0,1, 8,55,
			SpellElement::Lightning, {}, {6, 1.80,.22,.01,.01,.00,2.25}},
		{EnemyArchetype::Dragon, "Dragon", 50,70, 8,12, 2,4, 4,6, 8,70,
			SpellElement::Ice, {"Inferno", "Blizzard", "Corrupting Breath", "Chain Lightning", "Greater Heal"}, {8, 1.50,.16,.08,.15,.50,2.75}},
	};
	return definitions;
}
