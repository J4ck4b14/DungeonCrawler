#include "Enemy.h"
#include "utils/RNG.h"
#include <iostream>

Enemy::Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells,
	int xpReward, SpellElement weakness)
	: Entity(name, stats), xpReward_(xpReward), weakness_(weakness) {
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
		// Pick a random attack style
		int style = rng.NextInt(0, 2);
		switch (style) {
		case 0:
			action.attackStyle = AttackStyle::Slash;
			std::cout << "  " << name_ << " slashes at you!\n";
			break;
		case 1:
			action.attackStyle = AttackStyle::Thrust;
			std::cout << "  " << name_ << " thrusts precisely!\n";
			break;
		case 2:
			action.attackStyle = AttackStyle::Bash;
			std::cout << "  " << name_ << " winds up a heavy blow!\n";
			break;
		}
	}

	return action;
}

void Enemy::PrintStatus() const {
	PrintStatus(EnemyKnowledge::None);
}

void Enemy::PrintStatus(EnemyKnowledge knowledge) const {
	switch (knowledge) {
	case EnemyKnowledge::None:
		std::cout << name_ << " - HP: ???  | ATK: ???  | SPD: ???\n";
		break;
	case EnemyKnowledge::Approximate: {
		static RNG rng;
		// Show approximate values (within +/- 20%)
		auto approx = [&](int val) -> std::string {
			int fuzz = std::max(1, val / 5);
			int shown = val + rng.NextInt(-fuzz, fuzz);
			if (shown < 1) shown = 1;
			return "~" + std::to_string(shown);
		};
		std::cout << name_ << " - HP: " << approx(currentHp_) << "/" << approx(stats_.maxHp)
			<< "  | ATK: " << approx(stats_.atk)
			<< "  | SPD: " << approx(stats_.speed) << "\n";
		break;
	}
	case EnemyKnowledge::Partial:
		std::cout << name_ << " - HP: " << currentHp_ << "/" << stats_.maxHp
			<< "  | ATK: " << stats_.atk
			<< "  | SPD: " << stats_.speed << "\n";
		break;
	case EnemyKnowledge::Full: {
		std::cout << name_ << " - HP: " << currentHp_ << "/" << stats_.maxHp
			<< "  | ATK: " << stats_.atk
			<< "  | SPD: " << stats_.speed
			<< "  | INT: " << stats_.intelligence;
		// Show weakness
		Spell tmpSpell;
		tmpSpell.element = weakness_;
		std::cout << "  | WEAK TO: " << tmpSpell.GetElementName();
		// Show known spells
		if (!knownSpells_.empty()) {
			std::cout << "  | Spells: ";
			for (size_t i = 0; i < knownSpells_.size(); ++i) {
				if (i > 0) std::cout << ", ";
				std::cout << knownSpells_[i].name;
			}
		}
		std::cout << "\n";
		break;
	}
	}
}

int Enemy::GetXPReward() const { return xpReward_; }
SpellElement Enemy::GetWeakness() const { return weakness_; }