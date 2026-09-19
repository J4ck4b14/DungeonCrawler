// Entity.h
// --------
// Base class for all entities (player and enemies).
// Provides shared combat state: HP, mana, Strength, speed, intelligence,
// defending status, defense stance, power buffs, and known spells.
//
// ActionType: The possible actions an entity can take each turn.
//
// AttackStyle: Three physical attack variants:
//   Slash  -> 1.0x STR, 15% crit for 1.5x
//   Thrust -> 0.8x STR normally, 1.0x if target defends (ignores defense)
//   Bash   -> 1.3x STR, 15% chance to whiff + self-damage
//
// Players use reactive defense timing. DefenseStance remains shared state for
// enemy guards, whose stance can still affect the player's attack choice.
//
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
