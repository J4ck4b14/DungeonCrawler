// Enemy.cpp
// ---------
// Implementation of the Enemy class.
// Contains the simple enemy AI (40% spell if available, 20% defend, else attack
// with a random physical style) and the knowledge-tiered PrintStatus display.

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
			// Varied casting flavor
			std::vector<std::string> castLines = {
				name_ + " begins weaving dark energy...",
				name_ + " mutters an incantation under their breath...",
				name_ + "'s hands crackle with arcane power...",
				name_ + " channels a spell!",
				"A surge of magical energy radiates from " + name_ + "...",
			};
			std::cout << "  " << castLines[rng.NextInt(0, static_cast<int>(castLines.size()) - 1)] << "\n";
			return action;
		}
	}

	// 20% chance to defend, 80% to attack
	if (rng.Chance(0.2f)) {
		action.type = ActionType::Defend;
		// Pick a random stance — but DON'T reveal it
		int stance = rng.NextInt(0, 3);
		switch (stance) {
		case 0: action.defenseStance = DefenseStance::AntiSlash; break;
		case 1: action.defenseStance = DefenseStance::AntiThrust; break;
		case 2: action.defenseStance = DefenseStance::AntiBash; break;
		case 3: action.defenseStance = DefenseStance::AntiMagic; break;
		}
		// Vague defend flavor — never reveals what they're bracing for
		std::vector<std::string> defLines = {
			name_ + " raises their guard, watching carefully...",
			name_ + " shifts into a defensive posture.",
			name_ + " braces for your next move.",
			name_ + " hunkers down and waits.",
			name_ + " seems to be reading your movements...",
			name_ + " tenses up, eyes locked on you.",
		};
		std::cout << "  " << defLines[rng.NextInt(0, static_cast<int>(defLines.size()) - 1)] << "\n";
	}
	else {
		action.type = ActionType::Attack;
		// Pick a random attack style — reveal is vague, don't say the style directly
		int style = rng.NextInt(0, 2);
		switch (style) {
		case 0:
			action.attackStyle = AttackStyle::Slash;
			break;
		case 1:
			action.attackStyle = AttackStyle::Thrust;
			break;
		case 2:
			action.attackStyle = AttackStyle::Bash;
			break;
		}
		// Generic attack flavor — style is hidden
		std::vector<std::string> atkLines = {
			name_ + " lunges forward!",
			name_ + " attacks!",
			name_ + " closes in with violent intent!",
			name_ + " makes a move!",
			name_ + " strikes!",
			name_ + " rushes at you!",
			name_ + " charges with fury!",
		};
		std::cout << "  " << atkLines[rng.NextInt(0, static_cast<int>(atkLines.size()) - 1)] << "\n";
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