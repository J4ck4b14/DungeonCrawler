#pragma once
#include "Entity.h"
#include "items/Inventory.h"

class Player : public Entity {
public:
	Player(const std::string& name, const Stats& stats);

	TurnAction DecideTurn() override;

	Inventory& GetInventory();
	const Inventory& GetInventory() const;

	// Stat allocation at character creation
	static Stats AllocateStats(int pool);

	// Level-up: spend 3 points
	void AllocateLevelUpPoints();

	// Try to learn a spell from a defeated enemy
	bool TryLearnSpell(const Spell& spell);

	// XP system
	void GainXP(int amount);
	int GetXP() const;
	int GetLevel() const;
	int GetXPToNextLevel() const;

	void PrintStatus() const;

	// Access raw point allocations for level-up recalculation
	int GetRawHP() const;

private:
	Inventory inventory_;
	int xp_ = 0;
	int level_ = 1;
	int rawHp_ = 0;       // Track raw allocation points for recalc

	void CheckLevelUp();
	static int XPForLevel(int level);
};