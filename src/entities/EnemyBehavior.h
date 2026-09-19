#pragma once

#include "combat/CombatTypes.h"
#include "entities/EnemyDefinitions.h"
#include "status/StatusEffect.h"

#include <optional>
#include <vector>

class Enemy;
class RNG;

enum class BehaviorMove { Slash, Thrust, Bash, Defend, Cast, Hesitate };

struct EnemyStatusTrait {
	StatusType type = StatusType::Poison;
	double baseChance = 0.0;
	double chancePerRank = 0.0;
	double chanceCap = 0.0;
	int potencyRankDivisor = 1;
};

struct CommitmentTendencies {
	double physical = 0.4;
	double spell = 0.4;
	double guard = 0.5;
	double uncommittedRevision = 0.1;
};

struct EnemyBehaviorProfile {
	std::vector<BehaviorMove> rhythm;
	double irregularity = 0.0;
	bool adaptsToPlayer = false;
	std::optional<EnemyStatusTrait> onHitStatus;
	int regenerationBaseStacks = 0;
	int regenerationRankDivisor = 1;
	CommitmentTendencies commitment;
};

struct EnemyBehaviorState {
	int decisionsMade = 0;
	TurnAction lastPlayerAction;
	int repeatedPlayerActions = 0;
};

namespace EnemyBehavior {

const EnemyBehaviorProfile& Profile(EnemyArchetype archetype);
TurnAction Decide(const Enemy& enemy, EnemyBehaviorState& state, RNG& rng);
void ObservePlayerAction(EnemyBehaviorState& state, const TurnAction& action);
double OnHitStatusChance(const Enemy& enemy);
int OnHitStatusPotency(const Enemy& enemy);
double EarlyCommitmentChance(const Enemy& enemy, const TurnAction& action);
bool RollEarlyCommitment(const Enemy& enemy, const TurnAction& action, RNG& rng);
bool ShouldReviseUncommittedPlan(const Enemy& enemy, RNG& rng);
bool SameAction(const TurnAction& left, const TurnAction& right);

} // namespace EnemyBehavior
