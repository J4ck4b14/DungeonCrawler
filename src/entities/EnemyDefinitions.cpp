#include "EnemyDefinitions.h"

const std::vector<EnemyDefinition>& GetEnemyDefinitions() {
	static const std::vector<EnemyDefinition> definitions = {
		// Tier 1 (floor 1+)
		{"Slime",       8, 14,  1,  3, 1, 2, 0, 0, 1,  5, SpellElement::Fire,      {}},
		{"Rat",         6, 10,  1,  2, 3, 4, 0, 0, 1,  4, SpellElement::Fire,      {}},
		{"Skeleton",   10, 16,  2,  3, 1, 2, 0, 0, 1,  6, SpellElement::Arcane,    {}},
		{"Spider",      7, 12,  1,  3, 2, 3, 0, 0, 1,  5, SpellElement::Fire,      {}},
		{"Goblin",     10, 16,  2,  4, 2, 3, 1, 1, 1,  8, SpellElement::Ice,       {"Spark"}},

		// Tier 2 (floor 3+)
		{"Bandit",     14, 22,  3,  5, 2, 3, 1, 2, 3, 12, SpellElement::Lightning, {"Frost Bolt"}},
		{"Orc",        18, 28,  4,  6, 1, 2, 0, 1, 3, 14, SpellElement::Fire,      {}},
		{"Ghost",      12, 18,  2,  4, 3, 4, 2, 3, 3, 15, SpellElement::Arcane,    {"Shadow Bolt", "Frost Bolt"}},
		{"Witch",      14, 20,  2,  3, 2, 3, 3, 4, 3, 18, SpellElement::Shadow,    {"Fireball", "Heal", "Shadow Bolt"}},

		// Tier 3 (floor 5+)
		{"Troll",      25, 38,  5,  7, 1, 2, 0, 1, 5, 25, SpellElement::Fire,      {}},
		{"Werewolf",   22, 34,  5,  8, 3, 5, 0, 1, 5, 28, SpellElement::Ice,       {}},
		{"Vampire",    20, 30,  4,  6, 3, 4, 2, 4, 5, 30, SpellElement::Fire,      {"Shadow Bolt", "Heal", "Soul Drain"}},
		{"Dark Mage",  16, 24,  2,  4, 2, 3, 4, 5, 5, 32, SpellElement::Shadow,    {"Fireball", "Thunderbolt", "Ice Shard", "Heal"}},

		// Tier 4 (floor 8+)
		{"Demon",      35, 50,  6, 10, 2, 4, 3, 5, 8, 50, SpellElement::Ice,       {"Inferno", "Void Blast", "Soul Drain"}},
		{"Giant",      45, 60,  7, 11, 1, 2, 0, 1, 8, 55, SpellElement::Lightning, {}},
		{"Dragon",     50, 70,  8, 12, 2, 4, 4, 6, 8, 70, SpellElement::Ice,       {"Inferno", "Blizzard", "Chain Lightning", "Greater Heal"}},
	};
	return definitions;
}
