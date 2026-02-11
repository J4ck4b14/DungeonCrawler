#include "Enemy.h"
#include "utils/RNG.h"
#include <iostream>

Enemy::Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells)
	: Entity(name, stats) {
	for (const auto& spell : spells) {
		LearnSpell(spell);
	}
}

TurnAction Enemy::DecideTurn() {
	static RNG rng;
	TurnAction action;

	// Simple AI: if we have spells and mana, 40% chance to cast; otherwise attack
	if (!knownSpells_.empty()) {
		// Find usable spells (enough mana)
		std::vector<int> usable;
		for (size_t i = 0; i < knownSpells_.size(); ++i) {
			if (currentMana_ >= knownSpells_[i].manaCost) {
				usable.push_back(static_cast<int>(i));
			}
		}

		if (!usable.empty() && rng.Chance(0.4f)) {
			int pick = usable[rng.NextInt(0, static_cast<int>(usable.size()) - 1)];
			action.type = ActionType::CastSpell;
			action.spellIndex = pick;
			std::cout << "  " << name_ << " begins casting " << knownSpells_[pick].name << "!\n";
			return action;
		}
	}

	// 20% chance to defend, 80% to attack
	if (rng.Chance(0.2f)) {
		action.type = ActionType::Defend;
		std::cout << "  " << name_ << " braces for impact!\n";
	}
	else {
		action.type = ActionType::Attack;
		std::cout << "  " << name_ << " snarls and readies an attack!\n";
	}

	return action;
}

void Enemy::PrintStatus() const {
	std::cout << name_ << " — HP: " << currentHp_ << "/" << stats_.maxHp
		<< " | Mana: " << currentMana_ << "/" << stats_.maxMana
		<< " | ATK: " << stats_.atk
		<< " | SPD: " << stats_.speed << "\n";
}