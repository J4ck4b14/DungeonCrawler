// Enemy.cpp
// ---------
// Implementation of the Enemy class.
// Delegates decisions to the species behavior rules and owns per-specimen
// behavior memory, Rank, status traits, and knowledge-tiered display.
// The presentation layer describes the selected action to the player.

#include "Enemy.h"
#include "utils/RNG.h"
#include <iostream>
#include <algorithm>

Enemy::Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells,
	int xpReward, SpellElement weakness, int rank, EnemyArchetype archetype)
	: Entity(name, stats), xpReward_(xpReward), weakness_(weakness),
	  rank_(std::clamp(rank, 1, 50)), archetype_(archetype) {
	for (const auto& spell : spells) {
		LearnSpell(spell);
	}
	const EnemyBehaviorProfile& profile = EnemyBehavior::Profile(archetype_);
	if (profile.regenerationBaseStacks > 0) {
		const int stacks = profile.regenerationBaseStacks
			+ (rank_ - 1) / std::max(1, profile.regenerationRankDivisor);
		statuses_.ApplyRegeneration(stacks, 100000);
	}
}

TurnAction Enemy::DecideTurn() {
	static RNG rng;
	return DecideTurn(rng);
}

TurnAction Enemy::DecideTurn(RNG& rng) {
	return EnemyBehavior::Decide(*this, behaviorState_, rng);
}

void Enemy::ObservePlayerAction(const TurnAction& action) {
	EnemyBehavior::ObservePlayerAction(behaviorState_, action);
}

void Enemy::PrintStatus() const {
	PrintStatus(EnemyKnowledge::None);
}

void Enemy::PrintStatus(EnemyKnowledge knowledge) const {
	switch (knowledge) {
	case EnemyKnowledge::None:
		std::cout << name_ << " - HP: ???  | STR: ???  | SPD: ???\n";
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
			<< "  | STR: " << approx(stats_.strength)
			<< "  | SPD: " << approx(stats_.speed) << "\n";
		break;
	}
	case EnemyKnowledge::Partial:
		std::cout << name_ << " - HP: " << currentHp_ << "/" << stats_.maxHp
			<< "  | STR: " << stats_.strength
			<< "  | SPD: " << stats_.speed << "\n";
		break;
	case EnemyKnowledge::Full: {
		std::cout << name_ << " - HP: " << currentHp_ << "/" << stats_.maxHp
			<< "  | STR: " << stats_.strength
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
int Enemy::GetRank() const { return rank_; }
EnemyArchetype Enemy::GetArchetype() const { return archetype_; }

std::optional<StatusEffect> Enemy::RollOnHitStatus(RNG& rng) const {
	const EnemyBehaviorProfile& profile = EnemyBehavior::Profile(archetype_);
	if (!profile.onHitStatus || !rng.Chance(static_cast<float>(
		EnemyBehavior::OnHitStatusChance(*this)))) return std::nullopt;
	return StatusEffect{profile.onHitStatus->type,
		EnemyBehavior::OnHitStatusPotency(*this), 1};
}

void Enemy::SuppressRegeneration(int actions) {
	regenerationSuppressedActions_ = std::max(regenerationSuppressedActions_, actions);
}

bool Enemy::IsRegenerationSuppressed() const {
	return regenerationSuppressedActions_ > 0;
}

void Enemy::AdvanceRegenerationSuppression() {
	if (regenerationSuppressedActions_ > 0) --regenerationSuppressedActions_;
}
