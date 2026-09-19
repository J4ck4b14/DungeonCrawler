#pragma once

#include "entities/Enemy.h"
#include <string>

enum class IntentClarity {
	Veiled,
	Alert,
	Hinted,
	Clear,
	Exact
};

namespace EnemyIntent {

// Knowledge (including Inspect and, later, persistent Bestiary progress), INT,
// species readability, action type, and real commitment all contribute to the
// quality of a tell. Exact intent is never returned for an uncommitted action.
IntentClarity DetermineClarity(const Enemy& enemy, const TurnAction& action,
	EnemyKnowledge knowledge, int playerIntelligence, bool committed);
std::string Describe(const Enemy& enemy, const TurnAction& action,
	IntentClarity clarity);

} // namespace EnemyIntent
