#include "EnemyFactory.h"
#include "utils/RNG.h"
#include "combat/Spell.h"
#include <vector>

Enemy EnemyFactory::CreateEnemy(int dungeonLevel) {
	static RNG rng;

	struct EnemyTemplate {
		std::string name;
		int minHP, maxHP;
		int minATK, maxATK;
		int minSPD, maxSPD;
		int minINT, maxINT;
		int minLevel;               // Minimum dungeon level to appear
		std::vector<std::string> spellNames; // Spells this type can know
	};

	const std::vector<EnemyTemplate> templates = {
		// ?? Tier 1 (Level 1+) ??
		{"Slime",      20, 35,   3,  6,   1, 3,  0, 0,  1, {}},
		{"Rat",        15, 25,   2,  5,   4, 6,  0, 0,  1, {}},
		{"Skeleton",   25, 40,   4,  7,   2, 4,  0, 0,  1, {}},
		{"Spider",     18, 30,   3,  6,   3, 5,  0, 0,  1, {}},
		{"Goblin",     25, 40,   5,  8,   3, 5,  1, 2,  1, {"Spark"}},

		// ?? Tier 2 (Level 3+) ??
		{"Bandit",     35, 55,   7, 11,   3, 5,  1, 2,  3, {"Frost Bolt"}},
		{"Orc",        45, 65,   8, 13,   2, 4,  0, 1,  3, {}},
		{"Ghost",      30, 45,   5,  9,   4, 7,  3, 4,  3, {"Shadow Bolt", "Frost Bolt"}},
		{"Witch",      35, 50,   4,  7,   3, 5,  4, 5,  3, {"Fireball", "Heal", "Shadow Bolt"}},

		// ?? Tier 3 (Level 5+) ??
		{"Troll",      60, 90,  10, 16,   2, 3,  0, 1,  5, {}},
		{"Werewolf",   55, 80,  11, 17,   5, 8,  0, 1,  5, {}},
		{"Vampire",    50, 75,   9, 14,   4, 7,  3, 5,  5, {"Shadow Bolt", "Heal", "Soul Drain"}},
		{"Dark Mage",  40, 60,   5,  8,   3, 5,  5, 7,  5, {"Fireball", "Thunderbolt", "Ice Shard", "Heal"}},

		// ?? Tier 4 (Level 8+) ??
		{"Demon",      80, 120, 14, 22,   4, 7,  4, 6,  8, {"Inferno", "Void Blast", "Soul Drain"}},
		{"Giant",      100, 150, 16, 24,   1, 3,  0, 1,  8, {}},
		{"Dragon",     120, 180, 18, 28,   3, 6,  5, 8,  8, {"Inferno", "Blizzard", "Chain Lightning", "Greater Heal"}},
	};

	// Filter templates by dungeon level
	std::vector<size_t> eligible;
	for (size_t i = 0; i < templates.size(); ++i) {
		if (templates[i].minLevel <= dungeonLevel) {
			eligible.push_back(i);
		}
	}

	const auto& tmpl = templates[eligible[rng.NextInt(0, static_cast<int>(eligible.size()) - 1)]];

	// Scale stats slightly with dungeon level
	float scale = 1.0f + (dungeonLevel - 1) * 0.1f;

	Stats stats;
	stats.maxHp = static_cast<int>(rng.NextInt(tmpl.minHP, tmpl.maxHP) * scale);
	stats.atk = static_cast<int>(rng.NextInt(tmpl.minATK, tmpl.maxATK) * scale);
	stats.speed = rng.NextInt(tmpl.minSPD, tmpl.maxSPD);
	stats.intelligence = rng.NextInt(tmpl.minINT, tmpl.maxINT);
	stats.maxMana = stats.intelligence * 5;

	// Gather spells
	std::vector<Spell> spells;
	for (const auto& spellName : tmpl.spellNames) {
		const Spell* sp = FindSpell(spellName);
		if (sp && stats.intelligence >= sp->requiredIntelligence) {
			spells.push_back(*sp);
		}
	}

	return Enemy(tmpl.name, stats, spells);
}