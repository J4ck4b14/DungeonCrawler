#include "Player.h"
#include <iostream>
#include <limits>

Player::Player(const std::string& name, const Stats& stats)
	: Entity(name, stats), rawHp_(stats.hp) {}

int Player::XPForLevel(int level) {
	// XP thresholds: 30, 70, 120, 180, 250, ...
	return 10 * level + 20 * level;
}

Stats Player::AllocateStats(int pool) {
	Stats stats;

	std::cout << "\n=== CHARACTER CREATION ===\n";
	std::cout << "You have " << pool << " points to distribute.\n";
	std::cout << "Stats: Health, Attack, Speed, Intelligence\n";
	std::cout << "(Remaining points will go to Intelligence)\n\n";

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

	readStat("Health",  stats.hp);
	readStat("Attack",  stats.atk);
	readStat("Speed",   stats.speed);

	// Remaining points go to intelligence
	stats.intelligence = pool;
	std::cout << "Intelligence: " << stats.intelligence << " (remaining points)\n";

	// Convert raw allocation to actual combat values
	// Keep raw hp for later recalculation
	stats.atk = 2 + stats.atk * 2;       // Base 2 ATK + 2 per point
	stats.speed = 2 + stats.speed;        // Base 2 SPD + 1 per point
	stats.RecalculateDerived();            // Sets maxHp and maxMana

	std::cout << "\n";
	stats.Print("Your");
	std::cout << "\n";

	return stats;
}

void Player::AllocateLevelUpPoints() {
	int pool = 3;
	std::cout << "\n=== LEVEL UP! (Level " << level_ << ") ===\n";
	std::cout << "You have " << pool << " points to distribute.\n\n";

	auto readStat = [&](const std::string& label, int& current) {
		int value = -1;
		while (true) {
			std::cout << label << " [current: " << current << "] (0-" << pool << "): ";
			std::cin >> value;
			if (std::cin.fail() || value < 0 || value > pool) {
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::cout << "  Invalid.\n";
				continue;
			}
			current += value;
			pool -= value;
			break;
		}
	};

	// We work on raw allocation values and then recompute
	int atkBonus = 0, spdBonus = 0, intBonus = 0, hpBonus = 0;
	readStat("Health",       hpBonus);
	readStat("Attack",       atkBonus);
	readStat("Speed",        spdBonus);
	if (pool > 0) {
		intBonus = pool;
		std::cout << "Intelligence: +" << intBonus << " (remaining points)\n";
	}

	// Apply bonuses
	rawHp_ += hpBonus;
	stats_.hp = rawHp_;
	stats_.atk += atkBonus * 2;
	stats_.speed += spdBonus;
	stats_.intelligence += intBonus;
	stats_.RecalculateDerived();

	// Heal to new max on level up
	currentHp_ = stats_.maxHp;
	currentMana_ = stats_.maxMana;

	std::cout << "\n";
	stats_.Print("Updated");
	std::cout << "\n";
}

void Player::GainXP(int amount) {
	xp_ += amount;
	std::cout << "  +" << amount << " XP (" << xp_ << "/" << XPForLevel(level_) << ")\n";
	CheckLevelUp();
}

void Player::CheckLevelUp() {
	while (xp_ >= XPForLevel(level_)) {
		xp_ -= XPForLevel(level_);
		level_++;
		std::cout << "\n  *** LEVEL UP! You are now level " << level_ << "! ***\n";
		AllocateLevelUpPoints();
	}
}

int Player::GetXP() const { return xp_; }
int Player::GetLevel() const { return level_; }
int Player::GetXPToNextLevel() const { return XPForLevel(level_); }
int Player::GetRawHP() const { return rawHp_; }

TurnAction Player::DecideTurn() {
	TurnAction action;
	int choice = 0;

	std::cout << "\n" << name_ << "'s turn:\n"
		<< "  1. Attack\n"
		<< "  2. Defend\n"
		<< "  3. Cast Spell\n"
		<< "  4. Use Item\n"
		<< "  5. Inspect Enemy\n"
		<< "  > ";
	std::cin >> choice;

	while (std::cin.fail() || choice < 1 || choice > 5) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "  Invalid. Enter 1-5: ";
		std::cin >> choice;
	}

	switch (choice) {
	case 1: {
		action.type = ActionType::Attack;
		std::cout << "  Choose attack style:\n";
		std::cout << "    1. Slash  (balanced, 1.0x ATK)\n";
		std::cout << "    2. Thrust (precise, 0.8x ATK, ignores defense)\n";
		std::cout << "    3. Bash   (heavy, 1.3x ATK)\n";
		std::cout << "    > ";
		int atkChoice = 0;
		std::cin >> atkChoice;
		while (std::cin.fail() || atkChoice < 1 || atkChoice > 3) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "    Invalid. Enter 1-3: ";
			std::cin >> atkChoice;
		}
		switch (atkChoice) {
		case 1:
			action.attackStyle = AttackStyle::Slash;
			std::cout << "  " << name_ << " readies a slashing attack!\n";
			break;
		case 2:
			action.attackStyle = AttackStyle::Thrust;
			std::cout << "  " << name_ << " aims a precise thrust!\n";
			break;
		case 3:
			action.attackStyle = AttackStyle::Bash;
			std::cout << "  " << name_ << " winds up a heavy bash!\n";
			break;
		}
		break;
	}
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
	case 5:
		action.type = ActionType::Inspect;
		break;
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
	std::cout << name_ << " [Lv." << level_ << "] - HP: " << currentHp_ << "/" << stats_.maxHp
		<< " | Mana: " << currentMana_ << "/" << stats_.maxMana
		<< " | ATK: " << stats_.atk
		<< " | SPD: " << stats_.speed
		<< " | INT: " << stats_.intelligence
		<< " | XP: " << xp_ << "/" << XPForLevel(level_) << "\n";
}