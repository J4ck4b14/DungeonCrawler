#include "Player.h"
#include <iostream>
#include <limits>

Player::Player(const std::string& name, const Stats& stats)
	: Entity(name, stats) {}

Stats Player::AllocateStats() {
	Stats stats;
	int pool = 10;

	std::cout << "\n=== CHARACTER CREATION ===\n";
	std::cout << "You have " << pool << " points to distribute.\n";
	std::cout << "Stats: Health, Attack, Speed, Intelligence\n\n";

	auto readStat = [&](const std::string& label, int& stat) {
		int value = -1;
		while (true) {
			std::cout << label << " (0-" << pool << " remaining): ";
			std::cin >> value;
			if (std::cin.fail() || value < 0 || value > pool) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::cout << "  Invalid. Enter a number between 0 and " << pool << ".\n";
				continue;
			}
			stat = value;
			pool -= value;
			break;
		}
	};

	readStat("Health",       stats.hp);
	readStat("Attack",       stats.atk);
	readStat("Speed",        stats.speed);

	// Remaining points go to intelligence
	stats.intelligence = pool;
	std::cout << "Intelligence: " << stats.intelligence << " (remaining points)\n";

	// Convert raw allocation to actual values
	stats.atk = 5 + stats.atk * 3;       // Base 5 ATK + 3 per point
	stats.speed = 3 + stats.speed * 2;    // Base 3 SPD + 2 per point
	stats.RecalculateDerived();            // Sets maxHp and maxMana

	std::cout << "\n";
	stats.Print("Your");
	std::cout << "\n";

	return stats;
}

TurnAction Player::DecideTurn() {
	TurnAction action;
	int choice = 0;

	std::cout << "\n" << name_ << "'s turn:\n"
		<< "  1. Attack\n"
		<< "  2. Defend\n"
		<< "  3. Cast Spell\n"
		<< "  4. Use Item\n"
		<< "  > ";
	std::cin >> choice;

	while (std::cin.fail() || choice < 1 || choice > 4) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "  Invalid. Enter 1-4: ";
		std::cin >> choice;
	}

	switch (choice) {
	case 1:
		action.type = ActionType::Attack;
		std::cout << "  " << name_ << " readies a physical attack!\n";
		break;
	case 2:
		action.type = ActionType::Defend;
		std::cout << "  " << name_ << " braces for impact!\n";
		break;
	case 3: {
		if (knownSpells_.empty()) {
			std::cout << "  You don't know any spells! Attacking instead.\n";
			action.type = ActionType::Attack;
			break;
		}
		std::cout << "  Known spells:\n";
		for (size_t i = 0; i < knownSpells_.size(); ++i) {
			const auto& sp = knownSpells_[i];
			std::cout << "    " << (i + 1) << ". " << sp.name
				<< " [" << sp.GetElementName() << "] "
				<< "(Cost: " << sp.manaCost << " Mana, Power: " << sp.power << ")\n";
		}
		std::cout << "    0. Cancel\n  > ";
		int spellChoice = 0;
		std::cin >> spellChoice;
		if (spellChoice < 1 || spellChoice > static_cast<int>(knownSpells_.size())) {
			std::cout << "  Cancelled. Attacking instead.\n";
			action.type = ActionType::Attack;
			break;
		}
		int idx = spellChoice - 1;
		if (currentMana_ < knownSpells_[idx].manaCost) {
			std::cout << "  Not enough mana! Attacking instead.\n";
			action.type = ActionType::Attack;
			break;
		}
		action.type = ActionType::CastSpell;
		action.spellIndex = idx;
		std::cout << "  " << name_ << " prepares to cast " << knownSpells_[idx].name << "!\n";
		break;
	}
	case 4: {
		if (inventory_.IsEmpty()) {
			std::cout << "  Your inventory is empty! Attacking instead.\n";
			action.type = ActionType::Attack;
			break;
		}
		std::cout << "  Inventory:\n";
		inventory_.ListItems();
		std::cout << "    0. Cancel\n  > ";
		int itemChoice = 0;
		std::cin >> itemChoice;
		if (itemChoice < 1 || itemChoice > static_cast<int>(inventory_.Size())) {
			std::cout << "  Cancelled. Attacking instead.\n";
			action.type = ActionType::Attack;
			break;
		}
		action.type = ActionType::UseItem;
		action.itemIndex = itemChoice - 1;
		break;
	}
	}

	return action;
}

Inventory& Player::GetInventory() { return inventory_; }
const Inventory& Player::GetInventory() const { return inventory_; }

bool Player::TryLearnSpell(const Spell& spell) {
	if (stats_.intelligence >= spell.requiredIntelligence && !KnowsSpell(spell.name)) {
		LearnSpell(spell);
		std::cout << "  ** " << name_ << " learned " << spell.name << "! **\n";
		return true;
	}
	return false;
}

void Player::PrintStatus() const {
	std::cout << name_ << " — HP: " << currentHp_ << "/" << stats_.maxHp
		<< " | Mana: " << currentMana_ << "/" << stats_.maxMana
		<< " | ATK: " << stats_.atk
		<< " | SPD: " << stats_.speed
		<< " | INT: " << stats_.intelligence << "\n";
}