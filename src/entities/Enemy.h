// Enemy.h
// -------
// Enemy entity with elemental weakness and knowledge-gated stat display.
//
// EnemyKnowledge levels determine how much the player can see:
//   None        -> "???" for all stats (never seen this enemy type)
//   Approximate -> Fuzzy values with ~20% variance (seen this type on the floor)
//   Partial     -> Exact HP/ATK/SPD (decent Inspect roll)
//   Full        -> All stats + elemental weakness + known spells (18+ Inspect roll)
//
// Each enemy has a SpellElement weakness. Spells matching the weakness
// deal 1.5x damage ("super effective").

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