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
	static Stats AllocateStats();

	// Try to learn a spell from a defeated enemy
	bool TryLearnSpell(const Spell& spell);

	void PrintStatus() const;

private:
	Inventory inventory_;
};