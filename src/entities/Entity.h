// Entity.h
// --------
// Base class for all entities (player and enemies).
// Provides shared combat state: HP, mana, ATK, speed, intelligence,
// defending status, defense stance, attack buffs, and known spells.
//
// ActionType: The possible actions an entity can take each turn.
//
// AttackStyle: Three physical attack variants:
//   Slash  -> 1.0x ATK, 15% crit for 1.5x
//   Thrust -> 0.8x ATK normally, 1.0x if target defends (ignores defense)
//   Bash   -> 1.3x ATK, 15% chance to whiff + self-damage
//
// Players use reactive defense timing. DefenseStance remains shared state for
// enemy guards, whose stance can still affect the player's attack choice.
//
// AttackBuff: Temporary bonus damage on next N physical or magical attacks,
//   granted by Sharpen (physical) or Study (magical) at rest areas.

#pragma once
#include <string>
#include <vector>
#include "core/Stats.h"
#include "combat/Spell.h"
#include "combat/CombatTypes.h"

// Temporary attack bonus from rest area sharpening/studying
struct AttackBuff {
	int bonusDamage = 0;    // Flat bonus added per attack
	int remainingHits = 0;  // How many attacks the buff lasts
	bool isMagical = false; // true = spell buff, false = physical buff
};

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
	int GetATK() const;
	int GetSpeed() const;
	int GetIntelligence() const;
	int GetMana() const;
	int GetMaxMana() const;
	bool IsAlive() const;
	bool IsDefending() const;
	DefenseStance GetDefenseStance() const;
	const std::vector<Spell>& GetKnownSpells() const;

	// Modifiers
	void ReceiveDamage(int dmg);
	void Heal(int amount);
	void UseMana(int amount);
	void RestoreMana(int amount);
	void SetDefending(bool defending);
	void SetDefenseStance(DefenseStance stance);
	void LearnSpell(const Spell& spell);
	bool KnowsSpell(const std::string& name) const;

	// Speed-based action count
	int ActionsPerRound(int otherSpeed) const;

	// Attack buff system (from rest area sharpen/study)
	void ApplyAttackBuff(int bonus, int hits, bool magical);
	int ConsumeAttackBuff(bool magical); // Returns bonus and decrements remaining hits
	const AttackBuff& GetAttackBuff() const;
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
	AttackBuff attackBuff_;
	PowerBuff powerBuff_;
};
