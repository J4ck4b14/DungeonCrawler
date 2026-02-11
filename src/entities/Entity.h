#pragma once
#include <string>
#include <vector>
#include "core/Stats.h"
#include "combat/Spell.h"

enum class ActionType {
	Attack,
	Defend,
	CastSpell,
	UseItem,
	None
};

struct TurnAction {
	ActionType type = ActionType::None;
	int spellIndex = -1;  // Index into the entity's known spells
	int itemIndex = -1;   // Index into inventory (Player only)
};

class Entity {
public:
	Entity(const std::string& name, const Stats& stats);
	virtual ~Entity() = default;

	virtual TurnAction DecideTurn() = 0;

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
	const std::vector<Spell>& GetKnownSpells() const;

	// Modifiers
	void ReceiveDamage(int dmg);
	void Heal(int amount);
	void UseMana(int amount);
	void RestoreMana(int amount);
	void SetDefending(bool defending);
	void LearnSpell(const Spell& spell);
	bool KnowsSpell(const std::string& name) const;

	// Calculate how many actions this entity gets relative to another
	int ActionsPerRound(int otherSpeed) const;

protected:
	std::string name_;
	Stats stats_;
	int currentHp_;
	int currentMana_;
	bool defending_ = false;
	std::vector<Spell> knownSpells_;
};