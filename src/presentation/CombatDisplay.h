#pragma once

#include "entities/Enemy.h"
#include <string>
#include <vector>

class Player;

namespace CombatDisplay {

std::string MakeMeter(int current, int maximum, int width = 16);
void PrintEncounterIntro(const std::vector<Enemy>& enemies);
void PrintRoundHeader(int round, const Player& player,
	const std::vector<Enemy>& enemies,
	const std::vector<EnemyKnowledge>& knowledge,
	const std::vector<bool>& weaknessKnown,
	const std::vector<int>& initiativeOrder,
	const std::vector<TurnAction>& plannedActions,
	const std::vector<bool>& plannedIntentPending);
void PrintEnemyIntent(int enemyIndex, const Enemy& enemy, const TurnAction& action,
	EnemyKnowledge knowledge, int playerIntelligence, bool followUp = false);

} // namespace CombatDisplay
