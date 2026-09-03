// Player.cpp
// ----------
// Implementation of the Player class.
// Handles character creation, level-up, XP, combat turn menu with
// reactive defense state, attack style selection, and training system.

#include "Player.h"
#include "combat/CombatRules.h"
#include <iostream>
#include <limits>
#include <algorithm>

Player::Player(const std::string& name, const Stats& stats)
	: Entity(name, stats), rawHp_(stats.hp) {}

int Player::XPForLevel(int level) {
	// XP thresholds: 30, 70, 120, 180, 250, ... (5L^2 + 25L)
	return 5 * level * level + 25 * level;
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
	RecalcDerivedWithRelics();

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

// -- Training system --
int Player::GetTrainingPoints() const { return trainingPoints_; }
bool Player::CanTrain() const { return trainingPoints_ < MAX_TRAINING; }

// -- Death save counter --
int Player::GetDeathSaveCount() const { return deathSaveCount_; }
void Player::IncrementDeathSave() { deathSaveCount_++; }

// -- Relic system --

bool Player::HasRelic(RelicId id) const {
	for (RelicId r : relics_) if (r == id) return true;
	return false;
}

int Player::GetEffectiveManaCost(const Spell& spell) const {
	return CombatRules::EffectiveManaCost(
		spell.manaCost, HasRelic(RelicId::ArcaneBattery));
}

const std::vector<RelicId>& Player::GetRelics() const { return relics_; }

void Player::RecalcDerivedWithRelics() {
	stats_.RecalculateDerived();
	stats_.maxHp += relicMaxHpMod_;
	if (stats_.maxHp < 5) stats_.maxHp = 5;   // Never relic yourself below 5 HP
	if (currentHp_ > stats_.maxHp) currentHp_ = stats_.maxHp;
	if (currentMana_ > stats_.maxMana) currentMana_ = stats_.maxMana;
}

void Player::GrantRelic(RelicId id) {
	if (HasRelic(id)) return;
	relics_.push_back(id);

	switch (id) {
	case RelicId::BerserkersBrand:
		stats_.atk += 3;
		relicMaxHpMod_ -= 6;
		break;
	case RelicId::GiantsBelt:
		relicMaxHpMod_ += 18;
		stats_.atk = std::max(1, stats_.atk - 1);
		break;
	case RelicId::AdrenalGland:
		stats_.speed += 1;
		relicMaxHpMod_ -= 4;
		break;
	case RelicId::ScholarsMonocle:
		stats_.intelligence += 2;
		relicMaxHpMod_ -= 4;
		break;
	default:
		break; // Combat-hook relics have no immediate stat effect
	}
	RecalcDerivedWithRelics();
	// Taking Giant's Belt grants its HP immediately
	if (id == RelicId::GiantsBelt) currentHp_ = std::min(currentHp_ + 18, stats_.maxHp);
}

void Player::TrainStat(int statChoice) {
	if (!CanTrain()) return;
	trainingPoints_++;
	switch (statChoice) {
	case 1: // Health
		rawHp_++;
		stats_.hp = rawHp_;
		RecalcDerivedWithRelics();
		currentHp_ = std::min(currentHp_ + 5, stats_.maxHp); // Gain the 5 HP immediately
		std::cout << "  You train your endurance. +5 max HP!\n";
		break;
	case 2: // Attack
		stats_.atk += 1;
		std::cout << "  You practice combat drills. +1 ATK!\n";
		break;
	case 3: // Speed
		stats_.speed += 1;
		std::cout << "  You work on your footwork. +1 SPD!\n";
		break;
	case 4: // Intelligence
		stats_.intelligence += 1;
		RecalcDerivedWithRelics();
		currentMana_ = std::min(currentMana_ + 3, stats_.maxMana);
		std::cout << "  You meditate and expand your mind. +1 INT, +3 max Mana!\n";
		break;
	}
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
	if (!relics_.empty()) {
		std::cout << "  Relics: ";
		for (size_t i = 0; i < relics_.size(); ++i) {
			std::cout << GetRelicInfo(relics_[i]).name;
			if (i + 1 < relics_.size()) std::cout << ", ";
		}
		std::cout << "\n";
	}
}
