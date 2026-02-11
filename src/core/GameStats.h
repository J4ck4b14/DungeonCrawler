#pragma once
#include <string>
#include <map>
#include <iostream>

struct GameStats {
	int totalKills = 0;
	int roomsExplored = 0;
	int floorsCleared = 0;
	int trapsTriggered = 0;
	int trapsAvoided = 0;
	int chestsOpened = 0;
	int totalDamageDealt = 0;
	int totalDamageTaken = 0;
	int totalHealing = 0;
	int spellsCast = 0;
	int physicalAttacks = 0;

	// Track spell usage counts for "preferred spell"
	std::map<std::string, int> spellUsage;

	void RecordSpellCast(const std::string& spellName) {
		spellsCast++;
		spellUsage[spellName]++;
	}

	void RecordPhysicalAttack() {
		physicalAttacks++;
	}

	std::string GetPreferredSpell() const {
		if (spellUsage.empty()) return "(none)";
		std::string best;
		int bestCount = 0;
		for (const auto& pair : spellUsage) {
			if (pair.second > bestCount) {
				bestCount = pair.second;
				best = pair.first;
			}
		}
		return best + " (x" + std::to_string(bestCount) + ")";
	}

	void Print(const std::string& playerName, int playerLevel, int finalFloor, 
		bool victory, int spellCount) const {

		std::cout << "\n+==========================================+\n";
		if (victory)
			std::cout << "|           ADVENTURE COMPLETE             |\n";
		else
			std::cout << "|            FINAL REPORT                  |\n";
		std::cout << "+==========================================+\n\n";

		std::cout << "  Hero: " << playerName << "  (Level " << playerLevel << ")\n";
		std::cout << "  Floors cleared: " << floorsCleared << "\n";
		if (!victory)
			std::cout << "  Fell on floor: " << finalFloor << "\n";
		std::cout << "\n";

		std::cout << "  -- Combat --\n";
		std::cout << "  Enemies slain:     " << totalKills << "\n";
		std::cout << "  Physical attacks:  " << physicalAttacks << "\n";
		std::cout << "  Spells cast:       " << spellsCast << "\n";
		std::cout << "  Preferred spell:   " << GetPreferredSpell() << "\n";
		std::cout << "  Total spells known:" << spellCount << "\n";
		std::cout << "  Damage dealt:      " << totalDamageDealt << "\n";
		std::cout << "  Damage taken:      " << totalDamageTaken << "\n";
		std::cout << "  Healing done:      " << totalHealing << "\n";
		std::cout << "\n";

		std::cout << "  -- Exploration --\n";
		std::cout << "  Rooms explored:    " << roomsExplored << "\n";
		std::cout << "  Chests opened:     " << chestsOpened << "\n";
		std::cout << "  Traps triggered:   " << trapsTriggered << "\n";
		std::cout << "  Traps avoided:     " << trapsAvoided << "\n";
		std::cout << "\n+==========================================+\n\n";
	}
};
