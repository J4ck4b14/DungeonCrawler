#pragma once

#include "combat/Spell.h"
#include <string>
#include <vector>

enum class EnemyArchetype {
	Slime, Rat, Skeleton, Spider, Goblin, Bandit, Orc, Ghost,
	Witch, Troll, Werewolf, Vampire, DarkMage, Demon, Giant, Dragon
};

struct EnemyScaling {
	int rankOffset = 0;
	double hpPerRank = 0.0;
	double strengthPerRank = 0.0;
	double speedPerRank = 0.0;
	double intelligencePerRank = 0.0;
	double manaPerRank = 0.0;
	double xpPerRank = 0.0;
};

struct EnemyDefinition {
	EnemyArchetype archetype;
	std::string name;
	int minHp;
	int maxHp;
	int minStrength;
	int maxStrength;
	int minSpeed;
	int maxSpeed;
	int minIntelligence;
	int maxIntelligence;
	int minimumLevel;
	int baseXp;
	SpellElement weakness;
	std::vector<std::string> spellNames;
	EnemyScaling scaling;
};

const std::vector<EnemyDefinition>& GetEnemyDefinitions();
