// Per-specimen enemy state layered on top of the shared Entity combat state.
// EnemyKnowledge is a presentation tier supplied by inspection and the persistent
// Bestiary; the Enemy does not decide how that knowledge was earned.

#pragma once
#include "Entity.h"
#include "combat/Spell.h"
#include "entities/EnemyBehavior.h"

#include <optional>

// Increasing tiers reveal fuzzy core stats, exact core stats, then the full
// specimen readout. Persistent encounters can improve the tier between runs.
enum class EnemyKnowledge {
	None,
	Approximate,
	Partial,
	Full
};

class Enemy : public Entity {
public:
	Enemy(const std::string& name, const Stats& stats, const std::vector<Spell>& spells = {},
		int xpReward = 0, SpellElement weakness = SpellElement::Arcane,
		int rank = 1, EnemyArchetype archetype = EnemyArchetype::Slime);

	// Select an action without producing UI output.
	TurnAction DecideTurn();
	TurnAction DecideTurn(RNG& rng);
	void ObservePlayerAction(const TurnAction& action);

	// Print status based on how much the player knows
	void PrintStatus() const;
	void PrintStatus(EnemyKnowledge knowledge) const;

	int GetXPReward() const;
	SpellElement GetWeakness() const;
	int GetRank() const;
	EnemyArchetype GetArchetype() const;
	std::optional<StatusEffect> RollOnHitStatus(RNG& rng) const;
	void SuppressRegeneration(int actions = 2);
	bool IsRegenerationSuppressed() const;
	void AdvanceRegenerationSuppression();

private:
	int xpReward_;
	SpellElement weakness_;
	int rank_ = 1;
	EnemyArchetype archetype_ = EnemyArchetype::Slime;
	EnemyBehaviorState behaviorState_;
	int regenerationSuppressedActions_ = 0;
};
