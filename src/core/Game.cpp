#include "Game.h"
#include "combat/Spell.h"
#include <iostream>
#include <limits>
#include <string>

void Game::ShowTitle() const {
	std::cout << R"(
????????????????????????????????????????????
?                                          ?
?        D U N G E O N   C R A W L E R    ?
?                                          ?
?     Descend. Fight. Survive. Learn.      ?
?                                          ?
????????????????????????????????????????????
)" << std::endl;
}

void Game::Run() {
	ShowTitle();

	// ?? Character Creation ??
	std::string playerName;
	std::cout << "Enter your hero's name: ";
	std::getline(std::cin, playerName);
	if (playerName.empty()) playerName = "Hero";

	Stats playerStats = Player::AllocateStats();
	Player player(playerName, playerStats);

	// Give the player a starting spell if they have intelligence
	if (player.GetIntelligence() >= 1) {
		const Spell* startSpell = FindSpell("Magic Missile");
		if (startSpell) {
			player.LearnSpell(*startSpell);
			std::cout << "Your magical aptitude grants you the spell: Magic Missile!\n";
		}
	}
	if (player.GetIntelligence() >= 2) {
		const Spell* healSpell = FindSpell("Heal");
		if (healSpell) {
			player.LearnSpell(*healSpell);
			std::cout << "You also know: Heal!\n";
		}
	}

	// Give starting items
	player.GetInventory().AddItem(MakeHealthPotion());
	player.GetInventory().AddItem(MakeHealthPotion());
	player.GetInventory().AddItem(MakeManaPotion());
	std::cout << "You begin with 2 Health Potions and 1 Mana Potion.\n";

	// ?? Dungeon Loop ??
	Dungeon dungeon;

	while (true) {
		bool survived = dungeon.RunFloor(player);

		if (!survived) {
			std::cout << "\n????????????????????????????????????????????\n";
			std::cout << "?              GAME OVER                   ?\n";
			std::cout << "????????????????????????????????????????????\n";
			std::cout << "You fell on floor " << dungeon.GetCurrentLevel() << ".\n";
			std::cout << "Your legend ends here... for now.\n\n";
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
			std::cout << "\n????????????????????????????????????????????\n";
			std::cout << "?             VICTORY!                     ?\n";
			std::cout << "????????????????????????????????????????????\n";
			std::cout << playerName << " escapes the dungeon after clearing "
				<< (dungeon.GetCurrentLevel() - 1) << " floors!\n";
			player.PrintStatus();
			std::cout << "\nSpells learned:\n";
			for (const auto& s : player.GetKnownSpells()) {
				std::cout << "  - " << s.name << " [" << s.GetElementName() << "]\n";
			}
			std::cout << "\nThanks for playing DungeonCrawler!\n\n";
			break;
		}
	}
}
