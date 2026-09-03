#include "ExplorationDisplay.h"

#include "core/Relic.h"
#include "dungeon/Room.h"
#include "entities/Player.h"
#include "presentation/DisplayUtils.h"

#include <iostream>
#include <string>

namespace {

std::string RoomState(const Room& room) {
	if (!room.contentResolved) {
		return room.content == RoomContent::Staircase ? "Staircase" : "Unresolved";
	}
	switch (room.outcome) {
	case RoomOutcome::EnemyDefeated: return "Battle cleared";
	case RoomOutcome::ChestOpened: return "Chest opened";
	case RoomOutcome::Rested: return "Rest site used";
	case RoomOutcome::TrapTriggered: return "Trap triggered";
	case RoomOutcome::TrapDisarmed: return "Trap disarmed";
	case RoomOutcome::EmptySearched: return "Searched";
	case RoomOutcome::Unresolved: return "Cleared";
	}
	return "Cleared";
}

} // namespace

namespace ExplorationDisplay {

void PrintStatusPanel(int floor, int playerX, int playerY,
	int visitedRooms, int totalRooms, const Room& currentRoom,
	const Player& player) {
	std::cout << "\n+====================================================================+\n";
	std::cout << "| FLOOR " << floor << "  |  POSITION " << (playerX + 1) << "," << (playerY + 1)
		<< "  |  ROOMS " << visitedRooms << "/" << totalRooms
		<< "  |  " << RoomState(currentRoom) << "\n";
	std::cout << "+--------------------------------------------------------------------+\n";
	std::cout << "| " << player.GetName() << "  [Lv." << player.GetLevel() << "]\n";
	std::cout << "| HP " << DisplayUtils::MakeMeter(player.GetHP(), player.GetMaxHP(), 18)
		<< " " << player.GetHP() << "/" << player.GetMaxHP()
		<< "    MP " << DisplayUtils::MakeMeter(player.GetMana(), player.GetMaxMana(), 12)
		<< " " << player.GetMana() << "/" << player.GetMaxMana() << "\n";
	std::cout << "| XP " << DisplayUtils::MakeMeter(player.GetXP(), player.GetXPToNextLevel(), 18)
		<< " " << player.GetXP() << "/" << player.GetXPToNextLevel()
		<< "    ATK " << player.GetATK() << "  SPD " << player.GetSpeed()
		<< "  INT " << player.GetIntelligence() << "\n";
	std::cout << "| Inventory: " << player.GetInventory().Size()
		<< " item(s)  |  Relics: " << player.GetRelics().size() << "\n";

	const AttackBuff& attackBuff = player.GetAttackBuff();
	const PowerBuff& powerBuff = player.GetPowerBuff();
	if (attackBuff.remainingHits > 0 || powerBuff.remainingHits > 0) {
		std::cout << "| Active: ";
		bool needsSeparator = false;
		if (attackBuff.remainingHits > 0) {
			std::cout << "+" << attackBuff.bonusDamage
				<< (attackBuff.isMagical ? " spell" : " physical")
				<< " damage (" << attackBuff.remainingHits << " hits)";
			needsSeparator = true;
		}
		if (powerBuff.remainingHits > 0) {
			if (needsSeparator) std::cout << "  |  ";
			std::cout << "Empowered +" << powerBuff.percentBonus
				<< "% (" << powerBuff.remainingHits << " hits)";
		}
		std::cout << "\n";
	}
	std::cout << "+====================================================================+\n";
}

} // namespace ExplorationDisplay
