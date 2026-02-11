// EnemyFactory.cpp
// -----------------
// Creates enemies from tiered templates based on dungeon level.
//
// Templates are organized into 4 tiers:
//   Tier 1 (Level 1+): Slime, Rat, Skeleton, Spider, Goblin
//   Tier 2 (Level 3+): Bandit, Orc, Ghost, Witch
//   Tier 3 (Level 5+): Troll, Werewolf, Vampire, Dark Mage
//   Tier 4 (Level 8+): Demon, Giant, Dragon
//
// Each template defines stat ranges, spells, weakness element, and base XP.
// Stats scale by 8% per dungeon level above 1.

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
		int minLevel;
		int baseXP;
		SpellElement weakness;
		std::vector<std::string> spellNames;
	};

	const std::vector<EnemyTemplate> templates = {
		// Tier 1 (Level 1+) -- low stats, low reward
		{"Slime",      8, 14,    1,  3,   1, 2,  0, 0,  1,  5, SpellElement::Fire,      {}},
		{"Rat",        6, 10,    1,  2,   3, 4,  0, 0,  1,  4, SpellElement::Fire,      {}},
		{"Skeleton",  10, 16,    2,  3,   1, 2,  0, 0,  1,  6, SpellElement::Arcane,    {}},
		{"Spider",     7, 12,    1,  3,   2, 3,  0, 0,  1,  5, SpellElement::Fire,      {}},
		{"Goblin",    10, 16,    2,  4,   2, 3,  1, 1,  1,  8, SpellElement::Ice,       {"Spark"}},

		// Tier 2 (Level 3+)
		{"Bandit",    14, 22,    3,  5,   2, 3,  1, 2,  3, 12, SpellElement::Lightning, {"Frost Bolt"}},
		{"Orc",       18, 28,    4,  6,   1, 2,  0, 1,  3, 14, SpellElement::Fire,      {}},
		{"Ghost",     12, 18,    2,  4,   3, 4,  2, 3,  3, 15, SpellElement::Arcane,    {"Shadow Bolt", "Frost Bolt"}},
		{"Witch",     14, 20,    2,  3,   2, 3,  3, 4,  3, 18, SpellElement::Shadow,    {"Fireball", "Heal", "Shadow Bolt"}},

		// Tier 3 (Level 5+)
		{"Troll",     25, 38,    5,  7,   1, 2,  0, 1,  5, 25, SpellElement::Fire,      {}},
		{"Werewolf",  22, 34,    5,  8,   3, 5,  0, 1,  5, 28, SpellElement::Ice,       {}},
		{"Vampire",   20, 30,    4,  6,   3, 4,  2, 4,  5, 30, SpellElement::Fire,      {"Shadow Bolt", "Heal", "Soul Drain"}},
		{"Dark Mage", 16, 24,    2,  4,   2, 3,  4, 5,  5, 32, SpellElement::Shadow,    {"Fireball", "Thunderbolt", "Ice Shard", "Heal"}},

		// Tier 4 (Level 8+)
		{"Demon",     35, 50,    6, 10,   2, 4,  3, 5,  8, 50, SpellElement::Ice,       {"Inferno", "Void Blast", "Soul Drain"}},
		{"Giant",     45, 60,    7, 11,   1, 2,  0, 1,  8, 55, SpellElement::Lightning, {}},
		{"Dragon",    50, 70,    8, 12,   2, 4,  4, 6,  8, 70, SpellElement::Ice,       {"Inferno", "Blizzard", "Chain Lightning", "Greater Heal"}},
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
	float scale = 1.0f + (dungeonLevel - 1) * 0.08f;

	Stats stats;
	stats.maxHp = static_cast<int>(rng.NextInt(tmpl.minHP, tmpl.maxHP) * scale);
	stats.atk = static_cast<int>(rng.NextInt(tmpl.minATK, tmpl.maxATK) * scale);
	stats.speed = rng.NextInt(tmpl.minSPD, tmpl.maxSPD);
	stats.intelligence = rng.NextInt(tmpl.minINT, tmpl.maxINT);
	stats.maxMana = stats.intelligence * 3;

	int xp = static_cast<int>(tmpl.baseXP * scale);

	// Gather spells
	std::vector<Spell> spells;
	for (const auto& spellName : tmpl.spellNames) {
		const Spell* sp = FindSpell(spellName);
		if (sp && stats.intelligence >= sp->requiredIntelligence) {
			spells.push_back(*sp);
		}
	}

	return Enemy(tmpl.name, stats, spells, xp, tmpl.weakness);
}