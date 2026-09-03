#pragma once

#include "combat/Spell.h"
#include <string>
#include <vector>

struct EnemyDefinition {
	std::string name;
	int minHp;
	int maxHp;
	int minAttack;
	int maxAttack;
	int minSpeed;
	int maxSpeed;
	int minIntelligence;
	int maxIntelligence;
	int minimumLevel;
	int baseXp;
	SpellElement weakness;
	std::vector<std::string> spellNames;
};

const std::vector<EnemyDefinition>& GetEnemyDefinitions();
