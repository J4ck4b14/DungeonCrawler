// Entity.cpp
// ----------
// Implementation of the Entity base class.
// Handles damage (halved when defending), healing (capped at max),
// mana usage/restoration, spell learning, speed-based action calculation,
// defense stance, and temporary attack buffs.

#include "Entity.h"
#include <algorithm>

Entity::Entity(const std::string& name, const Stats& stats)
	: name_(name), stats_(stats),
	  currentHp_(stats.maxHp), currentMana_(stats.maxMana),
	  defending_(false), defenseStance_(DefenseStance::AntiSlash) {}

const std::string& Entity::GetName() const { return name_; }
int Entity::GetHP() const { return currentHp_; }
int Entity::GetMaxHP() const { return stats_.maxHp; }
int Entity::GetStrength() const { return stats_.strength; }
int Entity::GetSpeed() const { return stats_.speed; }
int Entity::GetIntelligence() const { return stats_.intelligence; }
int Entity::GetMana() const { return currentMana_; }
int Entity::GetMaxMana() const { return stats_.maxMana; }
bool Entity::IsAlive() const { return currentHp_ > 0; }
bool Entity::IsDefending() const { return defending_; }
DefenseStance Entity::GetDefenseStance() const { return defenseStance_; }
const std::vector<Spell>& Entity::GetKnownSpells() const { return knownSpells_; }
StatusContainer& Entity::GetStatuses() { return statuses_; }
const StatusContainer& Entity::GetStatuses() const { return statuses_; }

void Entity::ReceiveDamage(int dmg) {
	if (defending_) {
		dmg /= 2;
	}
	currentHp_ = std::max(0, currentHp_ - dmg);
}

void Entity::Heal(int amount) {
	currentHp_ = std::min(GetMaxHP(), currentHp_ + amount);
}

void Entity::UseMana(int amount) {
	currentMana_ = std::max(0, currentMana_ - amount);
}

void Entity::RestoreMana(int amount) {
	currentMana_ = std::min(GetMaxMana(), currentMana_ + amount);
}

void Entity::SetDefending(bool defending) {
	defending_ = defending;
}

void Entity::SetDefenseStance(DefenseStance stance) {
	defenseStance_ = stance;
}

void Entity::LearnSpell(const Spell& spell) {
	if (!KnowsSpell(spell.name)) {
		knownSpells_.push_back(spell);
	}
}

bool Entity::KnowsSpell(const std::string& name) const {
	for (const auto& s : knownSpells_) {
		if (s.name == name) return true;
	}
	return false;
}

StatusTurnResult Entity::ProcessStatusTurn(bool regenerationSuppressed) {
	StatusTurnResult result = statuses_.ProcessTurnStart(regenerationSuppressed);
	result.poisonDamage = std::min(currentHp_, result.poisonDamage);
	currentHp_ -= result.poisonDamage;
	result.burnDamage = std::min(currentHp_, result.burnDamage);
	currentHp_ -= result.burnDamage;
	result.bleedDamage = std::min(currentHp_, result.bleedDamage);
	currentHp_ -= result.bleedDamage;
	if (currentHp_ > 0) {
		result.regenerationHealing = std::min(
			stats_.maxHp - currentHp_, result.regenerationHealing);
		currentHp_ += result.regenerationHealing;
	}
	else {
		result.regenerationHealing = 0;
	}
	return result;
}

int Entity::ActionsPerRound(int otherSpeed) const {
	// You get a second action only when your speed is at least DOUBLE the
	// opponent's, and never more than 2 actions. Speed stays valuable
	// (turn order + the double-up threshold) without snowballing into
	// machine-gun rounds.
	if (otherSpeed <= 0) otherSpeed = 1;
	return (GetSpeed() >= otherSpeed * 2) ? 2 : 1;
}

void Entity::ApplyPowerBuff(int percentBonus, int hits) {
	percentBonus = std::max(0, percentBonus);
	hits = std::max(0, hits);
	if (powerBuff_.remainingHits > 0) {
		powerBuff_.percentBonus = std::max(powerBuff_.percentBonus, percentBonus);
		powerBuff_.remainingHits = std::max(powerBuff_.remainingHits, hits);
	}
	else {
		powerBuff_.percentBonus = percentBonus;
		powerBuff_.remainingHits = hits;
	}
}

int Entity::ConsumePowerBuff(int damage) {
	if (damage <= 0 || powerBuff_.remainingHits <= 0) return damage;
	const int empowered = damage * (100 + powerBuff_.percentBonus) / 100;
	--powerBuff_.remainingHits;
	if (powerBuff_.remainingHits == 0) powerBuff_.percentBonus = 0;
	return empowered;
}

const PowerBuff& Entity::GetPowerBuff() const { return powerBuff_; }
