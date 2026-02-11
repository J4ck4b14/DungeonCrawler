#include "Entity.h"
#include <algorithm>

Entity::Entity(const std::string& name, const Stats& stats)
	: name_(name), stats_(stats),
	  currentHp_(stats.maxHp), currentMana_(stats.maxMana),
	  defending_(false) {}

const std::string& Entity::GetName() const { return name_; }
int Entity::GetHP() const { return currentHp_; }
int Entity::GetMaxHP() const { return stats_.maxHp; }
int Entity::GetATK() const { return stats_.atk; }
int Entity::GetSpeed() const { return stats_.speed; }
int Entity::GetIntelligence() const { return stats_.intelligence; }
int Entity::GetMana() const { return currentMana_; }
int Entity::GetMaxMana() const { return stats_.maxMana; }
bool Entity::IsAlive() const { return currentHp_ > 0; }
bool Entity::IsDefending() const { return defending_; }
const std::vector<Spell>& Entity::GetKnownSpells() const { return knownSpells_; }

void Entity::ReceiveDamage(int dmg) {
	if (defending_) {
		dmg /= 2; // Defending halves incoming damage
	}
	currentHp_ = std::max(0, currentHp_ - dmg);
}

void Entity::Heal(int amount) {
	currentHp_ = std::min(stats_.maxHp, currentHp_ + amount);
}

void Entity::UseMana(int amount) {
	currentMana_ = std::max(0, currentMana_ - amount);
}

void Entity::RestoreMana(int amount) {
	currentMana_ = std::min(stats_.maxMana, currentMana_ + amount);
}

void Entity::SetDefending(bool defending) {
	defending_ = defending;
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

int Entity::ActionsPerRound(int otherSpeed) const {
	if (otherSpeed <= 0) return 1;
	return std::max(1, stats_.speed / otherSpeed);
}