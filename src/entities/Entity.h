// Shared mutable combat state for players and enemies. Attack resolution lives
// in CombatSystem; this class owns health/mana, statuses, learned spells, guard
// state and the small temporary buffs that both sides can use.

#pragma once
#include <string>
#include <vector>
#include "core/Stats.h"
#include "combat/Spell.h"
#include "combat/CombatTypes.h"
#include "status/StatusEffect.h"

struct PowerBuff {
	int percentBonus = 0;
	int remainingHits = 0;
};

class Entity {
public:
	Entity(const std::string& name, const Stats& stats);
	virtual ~Entity() = default;

	// Getters
	const std::string& GetName() const;
	int GetHP() const;
	int GetMaxHP() const;
	int GetStrength() const;
	virtual int GetSpeed() const;
	int GetIntelligence() const;
	int GetMana() const;
	virtual int GetMaxMana() const;
	bool IsAlive() const;
	bool IsDefending() const;
	DefenseStance GetDefenseStance() const;
	const std::vector<Spell>& GetKnownSpells() const;
	StatusContainer& GetStatuses();
	const StatusContainer& GetStatuses() const;

	// Modifiers
	void ReceiveDamage(int dmg);
	void Heal(int amount);
	void UseMana(int amount);
	void RestoreMana(int amount);
	void SetDefending(bool defending);
	void SetDefenseStance(DefenseStance stance);
	void LearnSpell(const Spell& spell);
	bool KnowsSpell(const std::string& name) const;
	StatusTurnResult ProcessStatusTurn(bool regenerationSuppressed = false);

	// Speed-based action count
	int ActionsPerRound(int otherSpeed) const;

	void ApplyPowerBuff(int percentBonus, int hits);
	int ConsumePowerBuff(int damage);
	const PowerBuff& GetPowerBuff() const;

protected:
	std::string name_;
	Stats stats_;
	int currentHp_;
	int currentMana_;
	bool defending_ = false;
	DefenseStance defenseStance_ = DefenseStance::AntiSlash;
	std::vector<Spell> knownSpells_;
	PowerBuff powerBuff_;
	StatusContainer statuses_;
};
