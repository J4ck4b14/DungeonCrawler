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

// Inspect results and persistent Bestiary knowledge share the same knowledge
// tier. INT, species readability and real commitment then determine how precise
// the tell can become. Uncommitted actions are never reported as exact.
IntentClarity DetermineClarity(const Enemy& enemy, const TurnAction& action,
	EnemyKnowledge knowledge, int playerIntelligence, bool committed);
std::string Describe(const Enemy& enemy, const TurnAction& action,
	IntentClarity clarity);

} // namespace EnemyIntent
