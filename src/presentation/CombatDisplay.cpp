#include "CombatDisplay.h"

#include "entities/Player.h"
#include "presentation/DisplayUtils.h"
#include "presentation/EnemyIntent.h"
#include <iostream>

namespace CombatDisplay {

std::string MakeMeter(int current, int maximum, int width) {
	return DisplayUtils::MakeMeter(current, maximum, width);
}

void PrintEncounterIntro(const std::vector<Enemy>& enemies) {
	std::cout << "\n+====================================================================+\n";
	if (enemies.size() == 1) {
		std::cout << "  A " << enemies.front().GetName() << " appears!\n";
	}
	else {
		std::cout << "  AMBUSH! " << enemies.size() << " enemies close in!\n";
		for (size_t i = 0; i < enemies.size(); ++i) {
			std::cout << "    [E" << (i + 1) << "] " << enemies[i].GetName() << "\n";
		}
	}
	std::cout << "+====================================================================+\n";
}

void PrintRoundHeader(int round, const Player& player,
	const std::vector<Enemy>& enemies,
	const std::vector<EnemyKnowledge>& knowledge,
	const std::vector<bool>& weaknessKnown,
	const std::vector<int>& initiativeOrder,
	const std::vector<TurnAction>& plannedActions,
	const std::vector<bool>& plannedIntentPending) {
	std::cout << "\n+====================================================================+\n";
	std::cout << "| ROUND " << round << "  |  TURN ORDER: ";
	for (size_t i = 0; i < initiativeOrder.size(); ++i) {
		if (i > 0) std::cout << " > ";
		const int actor = initiativeOrder[i];
		if (actor < 0) std::cout << player.GetName();
		else std::cout << "E" << (actor + 1);
	}
	std::cout << "\n";
	std::cout << "+-- HERO -------------------------------------------------------------+\n";
	std::cout << "| " << player.GetName() << " [Lv." << player.GetLevel() << "]"
		<< "  ATK " << player.GetATK() << "  SPD " << player.GetSpeed()
		<< "  INT " << player.GetIntelligence() << "\n";
	std::cout << "| HP " << MakeMeter(player.GetHP(), player.GetMaxHP(), 18)
		<< " " << player.GetHP() << "/" << player.GetMaxHP() << "\n";
	std::cout << "| MP " << MakeMeter(player.GetMana(), player.GetMaxMana(), 18)
		<< " " << player.GetMana() << "/" << player.GetMaxMana() << "\n";
	const PowerBuff& powerBuff = player.GetPowerBuff();
	if (player.IsDefending()) {
		std::cout << "| STATUS: REACTIVE GUARD READY\n";
	}
	if (powerBuff.remainingHits > 0) {
		std::cout << "| STATUS: EMPOWERED +" << powerBuff.percentBonus
			<< "% damage (" << powerBuff.remainingHits << " hits)\n";
	}
	std::cout << "+-- HOSTILES ---------------------------------------------------------+\n";
	for (size_t i = 0; i < enemies.size(); ++i) {
		const Enemy& enemy = enemies[i];
		std::cout << "| [E" << (i + 1) << "] ";
		if (!enemy.IsAlive()) {
			std::cout << enemy.GetName() << "  [DEFEATED]\n";
			continue;
		}
		enemy.PrintStatus(knowledge[i]);
		if (knowledge[i] >= EnemyKnowledge::Partial) {
			std::cout << "|      HP " << MakeMeter(enemy.GetHP(), enemy.GetMaxHP(), 14)
				<< " " << enemy.GetHP() << "/" << enemy.GetMaxHP() << "\n";
		}
		if (weaknessKnown[i] && knowledge[i] != EnemyKnowledge::Full) {
			Spell weakness;
			weakness.element = enemy.GetWeakness();
			std::cout << "|      WEAKNESS: " << weakness.GetElementName() << "\n";
		}
		if (i < plannedActions.size() && i < plannedIntentPending.size()
			&& plannedIntentPending[i]) {
			const IntentClarity clarity = EnemyIntent::DetermineClarity(
				knowledge[i], player.GetIntelligence());
			std::cout << "|      INTENT: "
				<< EnemyIntent::Describe(enemy, plannedActions[i], clarity) << "\n";
		}
	}
	std::cout << "+====================================================================+\n";
}

void PrintEnemyIntent(int enemyIndex, const Enemy& enemy, const TurnAction& action,
	EnemyKnowledge knowledge, int playerIntelligence, bool followUp) {
	const IntentClarity clarity = EnemyIntent::DetermineClarity(
		knowledge, playerIntelligence);
	std::cout << "    [E" << (enemyIndex + 1) << "] "
		<< (followUp ? "FOLLOW-UP: " : "")
		<< EnemyIntent::Describe(enemy, action, clarity) << "\n";
}

} // namespace CombatDisplay
