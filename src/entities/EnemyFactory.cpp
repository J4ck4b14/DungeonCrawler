#include "EnemyFactory.h"

#include "EnemyDefinitions.h"
#include "combat/Spell.h"
#include "core/DevMode.h"
#include "utils/RNG.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
int Grow(int base, int rankDelta, double perRank) {
	return base + static_cast<int>(std::floor(rankDelta * perRank));
}
}

int EnemyFactory::CalculateRank(const EnemyDefinition& definition,
	int dungeonLevel, int specimenVariance) {
	const int depthRank = 1 + 2 * std::max(0, dungeonLevel - 1);
	return std::clamp(depthRank + definition.scaling.rankOffset
		+ specimenVariance, 1, 50);
}

Enemy EnemyFactory::CreateEnemy(int dungeonLevel) {
	static RNG rng;
	return CreateEnemy(dungeonLevel, rng);
}

Enemy EnemyFactory::CreateEnemy(int dungeonLevel, RNG& rng) {
	dungeonLevel = std::max(1, dungeonLevel);
	const auto& definitions = GetEnemyDefinitions();
	std::vector<std::size_t> eligible;
	for (std::size_t i = 0; i < definitions.size(); ++i) {
		if (definitions[i].minimumLevel <= dungeonLevel
			&& dungeonLevel - definitions[i].minimumLevel < 5) eligible.push_back(i);
	}
	if (eligible.empty()) {
		for (std::size_t i = 0; i < definitions.size(); ++i) {
			if (definitions[i].minimumLevel <= dungeonLevel) eligible.push_back(i);
		}
	}

	int totalWeight = 0;
	for (std::size_t index : eligible) {
		totalWeight += definitions[index].minimumLevel * definitions[index].minimumLevel;
	}
	int roll = rng.NextInt(1, totalWeight);
	std::size_t chosen = eligible.back();
	for (std::size_t index : eligible) {
		roll -= definitions[index].minimumLevel * definitions[index].minimumLevel;
		if (roll <= 0) { chosen = index; break; }
	}
	return CreateEnemy(definitions[chosen], dungeonLevel, rng);
}

Enemy EnemyFactory::CreateEnemy(const EnemyDefinition& definition,
	int dungeonLevel, RNG& rng) {
	const int rank = CalculateRank(definition, dungeonLevel, rng.NextInt(-1, 2));
	const int delta = rank - 1;
	const EnemyScaling& growth = definition.scaling;

	Stats stats;
	stats.maxHp = std::max(1, Grow(rng.NextInt(definition.minHp,
		definition.maxHp), delta, growth.hpPerRank));
	stats.strength = std::max(1, Grow(rng.NextInt(definition.minStrength,
		definition.maxStrength), delta, growth.strengthPerRank));
	stats.speed = std::max(1, Grow(rng.NextInt(definition.minSpeed,
		definition.maxSpeed), delta, growth.speedPerRank));
	stats.intelligence = std::max(0, Grow(rng.NextInt(definition.minIntelligence,
		definition.maxIntelligence), delta, growth.intelligencePerRank));
	stats.maxMana = std::max(0, stats.intelligence * 3
		+ static_cast<int>(std::floor(delta * growth.manaPerRank)));
	int xp = definition.baseXp + static_cast<int>(std::floor(
		std::pow(static_cast<double>(delta), 0.82) * growth.xpPerRank));

	if (DevMode::IsEnabled()) {
		const float scale = DevMode::GetEnemyScale();
		stats.maxHp = std::max(1, static_cast<int>(stats.maxHp * scale));
		stats.strength = std::max(1, static_cast<int>(stats.strength * scale));
		xp = std::max(1, static_cast<int>(xp * scale));
	}

	std::vector<Spell> spells;
	for (const std::string& spellName : definition.spellNames) {
		const Spell* spell = FindSpell(spellName);
		if (spell && stats.intelligence >= spell->requiredIntelligence) {
			spells.push_back(*spell);
		}
	}

	return Enemy(definition.name, stats, spells, std::max(1, xp),
		definition.weakness, rank, definition.archetype);
}
