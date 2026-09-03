#pragma once

#include "entities/Enemy.h"
#include <string>

enum class IntentClarity {
	Veiled,
	Hinted,
	Clear,
	Exact
};

namespace EnemyIntent {

IntentClarity DetermineClarity(EnemyKnowledge knowledge, int playerIntelligence);
std::string Describe(const Enemy& enemy, const TurnAction& action,
	IntentClarity clarity);

} // namespace EnemyIntent
