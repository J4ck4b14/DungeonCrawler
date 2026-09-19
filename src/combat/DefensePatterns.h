#pragma once

#include "DefenseRules.h"

struct Spell;

namespace DefensePatterns {

DefenseChallenge Build(EnemyArchetype archetype, int enemyRank,
	const TurnAction& action, const Spell* spell, int patternVariant);

} // namespace DefensePatterns
