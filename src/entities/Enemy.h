#pragma once
#include "Entity.h"
#include "combat/Spell.h"

// How much the player knows about this enemy
enum class EnemyKnowledge {
	None,        // Never inspected or seen before
	Approximate, // Seen this type before on the floor
	Partial,     // Inspected with a decent roll
	Full         // Inspected with 18+ roll
};

class Enemy : public Entity {
public:
	Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells = {},
		int xpReward = 0, SpellElement weakness = SpellElement::Arcane);

	TurnAction DecideTurn() override;

	// Print status based on how much the player knows
	void PrintStatus() const;
	void PrintStatus(EnemyKnowledge knowledge) const;

	int GetXPReward() const;
	SpellElement GetWeakness() const;

private:
	int xpReward_;
	SpellElement weakness_;
};