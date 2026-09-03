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
#include "EnemyDefinitions.h"
#include "utils/RNG.h"
#include "combat/Spell.h"
#include <vector>
#include <cmath>
#include "core/DevMode.h"

Enemy EnemyFactory::CreateEnemy(int dungeonLevel) {
	static RNG rng;

	const auto& templates = GetEnemyDefinitions();

	// Filter templates by dungeon level, retiring badly outleveled tiers:
	// once you are 5+ floors past a tier's unlock, it stops spawning
	// (tier 1 disappears at floor 6, tier 2 at floor 8, tier 3 at floor 10).
	std::vector<size_t> eligible;
	for (size_t i = 0; i < templates.size(); ++i) {
		if (templates[i].minimumLevel <= dungeonLevel
			&& dungeonLevel - templates[i].minimumLevel < 5) {
			eligible.push_back(i);
		}
	}
	// Safety net: never allow an empty pool
	if (eligible.empty()) {
		for (size_t i = 0; i < templates.size(); ++i) {
			if (templates[i].minimumLevel <= dungeonLevel) eligible.push_back(i);
		}
	}

	// Weighted pick: higher-tier templates are proportionally more likely,
	// so the newest threats dominate each floor instead of tier-1 filler.
	int totalWeight = 0;
	for (size_t idx : eligible) totalWeight += templates[idx].minimumLevel * templates[idx].minimumLevel;
	int roll = rng.NextInt(1, totalWeight);
	size_t chosen = eligible.back();
	for (size_t idx : eligible) {
		roll -= templates[idx].minimumLevel * templates[idx].minimumLevel;
		if (roll <= 0) { chosen = idx; break; }
	}
	const auto& tmpl = templates[chosen];

	// Enemy STATS compound 10% per floor (exponential), while XP rewards
	// scale only linearly. Player power can't keep pace forever -- the
	// dungeon eventually forms a wall. Every floor you descend past your
	// limit is greed; escaping alive is the victory. That's the run.
	float statScale = std::pow(1.10f, static_cast<float>(dungeonLevel - 1));
	float xpScale = 1.0f + (dungeonLevel - 1) * 0.10f;

	Stats stats;
	stats.maxHp = static_cast<int>(rng.NextInt(tmpl.minHp, tmpl.maxHp) * statScale);
	stats.atk = static_cast<int>(rng.NextInt(tmpl.minAttack, tmpl.maxAttack) * statScale);
	stats.speed = rng.NextInt(tmpl.minSpeed, tmpl.maxSpeed);
	stats.intelligence = rng.NextInt(tmpl.minIntelligence, tmpl.maxIntelligence);
	stats.maxMana = stats.intelligence * 3;

	int xp = static_cast<int>(tmpl.baseXp * xpScale);

	// Apply dev-mode scaling to make enemies stronger / yield more XP during balancing
	if (DevMode::IsEnabled()) {
		float s = DevMode::GetEnemyScale();
		stats.maxHp = static_cast<int>(stats.maxHp * s);
		stats.atk = static_cast<int>(stats.atk * s);
		xp = static_cast<int>(xp * s);
		// ensure sane minima
		if (stats.maxHp < 1) stats.maxHp = 1;
		if (stats.atk < 1) stats.atk = 1;
		if (xp < 1) xp = 1;
	}

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
