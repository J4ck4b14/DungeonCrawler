// Player.h
// --------
// The player entity. Extends Entity with:
//   - Inventory for consumable items
//   - XP / leveling system (3 stat points per level)
//   - Interactive DecideTurn() with attack style submenu,
//     directional defense stance, spell selection, item use, enemy inspection
//   - Character creation via AllocateStats() (distributes 5 starting points)
//   - Training counter for rest area stat boosts (capped at 3 total)

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

	// Training system (rest areas): capped total across the run
	int GetTrainingPoints() const;
	bool CanTrain() const;
	void TrainStat(int statChoice); // 1=HP, 2=ATK, 3=SPD, 4=INT

	// Death save counter: each use makes the next QTE harder
	int GetDeathSaveCount() const;
	void IncrementDeathSave();

private:
	Inventory inventory_;
	int xp_ = 0;
	int level_ = 1;
	int rawHp_ = 0;
	int trainingPoints_ = 0;      // Total training bonuses used (cap 3)
	static const int MAX_TRAINING = 3;
	int deathSaveCount_ = 0;      // How many times the player has cheated death

	void CheckLevelUp();
	static int XPForLevel(int level);
};