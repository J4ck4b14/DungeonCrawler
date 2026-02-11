#include "Game.h"
#include "combat/Spell.h"
#include "items/Item.h"
#include "utils/Console.h"
#include <iostream>
#include <limits>
#include <string>

void Game::ShowTitle() const {
	Console::Clear();
	Console::PrintSlow(R"(
+==========================================+
|                                          |
|        D U N G E O N   C R A W L E R     |
|                                          |
|    Explore. Perceive. Fight. Survive.    |
|                                          |
+==========================================+
)");
}

void Game::Run() {
	ShowTitle();

	// -- Character Creation --
	std::string playerName;
	std::cout << "Enter your hero's name: ";
	std::getline(std::cin, playerName);
	if (playerName.empty()) playerName = "Hero";

	Stats playerStats = Player::AllocateStats(5);
	Player player(playerName, playerStats);

	// Give the player a starting spell if they have intelligence
	if (player.GetIntelligence() >= 1) {
		const Spell* startSpell = FindSpell("Magic Missile");
		if (startSpell) {
			player.LearnSpell(*startSpell);
			Console::PrintSlow("Your magical aptitude grants you the spell: Magic Missile!");
		}
	}
	if (player.GetIntelligence() >= 2) {
		const Spell* healSpell = FindSpell("Heal");
		if (healSpell) {
			player.LearnSpell(*healSpell);
			Console::PrintSlow("You also know: Heal!");
		}
	}

	// Give starting items
	player.GetInventory().AddItem(MakeHealthPotion());
	player.GetInventory().AddItem(MakeManaPotion());
	Console::PrintSlow("You begin with 1 Health Potion and 1 Mana Potion.");
	Console::PrintSlow("");
	Console::PrintSlow("TIP: Use Perception checks to scout rooms before entering!");
	Console::PrintSlow("     Higher Intelligence = better perception rolls.");
	Console::PrintSlow("     In combat, choose your attack style wisely:");
	Console::PrintSlow("       Slash (balanced), Thrust (ignores defense), Bash (heavy hit)");
	Console::PrintSlow("");

	// -- Dungeon Loop --
	Dungeon dungeon;

	while (true) {
		bool survived = dungeon.RunFloor(player);

		if (!survived) {
			Console::Clear();
			Console::PrintSlow("\n+==========================================+");
			Console::PrintSlow("|              GAME OVER                   |");
			Console::PrintSlow("+==========================================+");
			Console::PrintSlow("You fell on floor " + std::to_string(dungeon.GetCurrentLevel()) + ".");
			Console::PrintSlow("Your legend ends here... for now.");
			Console::PrintSlow("");

			// Show final stats
			dungeon.GetStats().Print(
				playerName, player.GetLevel(), dungeon.GetCurrentLevel(),
				false, static_cast<int>(player.GetKnownSpells().size()));

			player.PrintStatus();
			std::cout << "\nSpells known:\n";
			const auto& spells = player.GetKnownSpells();
			if (spells.empty()) {
				std::cout << "  (none)\n";
			}
			else {
				for (const auto& s : spells) {
					std::cout << "  - " << s.name << " [" << s.GetElementName() << "]\n";
				}
			}
			std::cout << "\n";
			break;
		}

		std::cout << "\nContinue to the next floor?\n"
			<< "  1. Yes, descend deeper\n"
			<< "  2. No, escape the dungeon\n"
			<< "  > ";
		int choice = 0;
		std::cin >> choice;
		while (std::cin.fail() || (choice != 1 && choice != 2)) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "  Enter 1 or 2: ";
			std::cin >> choice;
		}

		if (choice == 2) {
			Console::Clear();
			Console::PrintSlow("\n+==========================================+");
			Console::PrintSlow("|             VICTORY!                     |");
			Console::PrintSlow("+==========================================+");
			Console::PrintSlow(playerName + " escapes the dungeon after clearing "
				+ std::to_string(dungeon.GetCurrentLevel() - 1) + " floors!");
			Console::PrintSlow("");

			// Show final stats
			dungeon.GetStats().Print(
				playerName, player.GetLevel(), dungeon.GetCurrentLevel() - 1,
				true, static_cast<int>(player.GetKnownSpells().size()));

			player.PrintStatus();
			std::cout << "\nSpells known:\n";
			const auto& spells = player.GetKnownSpells();
			if (spells.empty()) {
				std::cout << "  (none)\n";
			}
			else {
				for (const auto& s : spells) {
					std::cout << "  - " << s.name << " [" << s.GetElementName() << "]\n";
				}
			}
			Console::PrintSlow("\nThanks for playing DungeonCrawler!");
			std::cout << "\n";
			break;
		}
	}
}
