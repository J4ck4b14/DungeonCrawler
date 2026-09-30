#include "EnemyBehavior.h"

#include "entities/Enemy.h"
#include "utils/RNG.h"

#include <algorithm>
#include <array>

namespace {
using Move = BehaviorMove;

// Profile order is part of the data contract: each slot maps directly to the
// matching EnemyArchetype value. Count keeps additions from silently indexing
// the wrong behavior profile.
constexpr std::size_t kArchetypeCount = static_cast<std::size_t>(EnemyArchetype::Count);
const std::array<EnemyBehaviorProfile, kArchetypeCount> Profiles = {{
	{{Move::Hesitate, Move::Bash, Move::Slash}, .20, false, std::nullopt, 0, 1, {.45,.20,.50,.15}},
	{{Move::Thrust, Move::Thrust, Move::Hesitate, Move::Slash}, .10, false, std::nullopt, 0, 1, {.25,.15,.40,.25}},
	{{Move::Slash, Move::Slash, Move::Defend}, .00, false, std::nullopt, 0, 1, {.70,.40,.75,.02}},
	{{Move::Thrust, Move::Defend, Move::Thrust, Move::Bash}, .15, false,
		EnemyStatusTrait{StatusType::Poison, .22, .005, .55, 12}, 0, 1, {.35,.25,.45,.15}},
	{{Move::Defend, Move::Thrust, Move::Hesitate, Move::Bash}, .30, true, std::nullopt, 0, 1, {.18,.15,.30,.55}},
	{{Move::Slash, Move::Defend, Move::Thrust, Move::Cast}, .08, true,
		EnemyStatusTrait{StatusType::Bleed, .16, .004, .40, 16}, 0, 1, {.15,.25,.35,.65}},
	{{Move::Bash, Move::Hesitate, Move::Slash}, .00, false, std::nullopt, 0, 1, {.78,.25,.65,.08}},
	{{Move::Cast, Move::Hesitate, Move::Thrust, Move::Cast}, .18, false, std::nullopt, 0, 1, {.25,.40,.30,.20}},
	{{Move::Cast, Move::Cast, Move::Hesitate, Move::Cast}, .00, false, std::nullopt, 0, 1, {.25,.75,.35,.12}},
	{{Move::Bash, Move::Hesitate, Move::Slash, Move::Defend}, .00, false, std::nullopt, 1, 15, {.75,.20,.70,.05}},
	{{Move::Slash, Move::Thrust, Move::Bash, Move::Hesitate}, .08, false, std::nullopt, 0, 1, {.18,.15,.25,.28}},
	{{Move::Defend, Move::Cast, Move::Thrust, Move::Cast}, .05, true, std::nullopt, 0, 1, {.35,.55,.45,.35}},
	{{Move::Cast, Move::Hesitate, Move::Cast, Move::Defend, Move::Cast}, .00, false, std::nullopt, 0, 1, {.15,.42,.35,.18}},
	{{Move::Bash, Move::Cast, Move::Slash, Move::Cast}, .05, false,
		EnemyStatusTrait{StatusType::Burn, .24, .006, .60, 14}, 0, 1, {.62,.68,.55,.10}},
	{{Move::Hesitate, Move::Bash, Move::Hesitate, Move::Slash}, .00, false, std::nullopt, 0, 1, {.90,.20,.75,.02}},
	{{Move::Cast, Move::Bash, Move::Cast, Move::Defend, Move::Cast}, .08, true,
		EnemyStatusTrait{StatusType::Burn, .18, .004, .45, 18}, 0, 1, {.58,.62,.55,.12}},
}};
static_assert(Profiles.size() == kArchetypeCount);

bool ActionsMatch(const TurnAction& left, const TurnAction& right) {
	if (left.type != right.type) return false;
	if (left.type == ActionType::Attack) return left.attackStyle == right.attackStyle;
	if (left.type == ActionType::CastSpell) return true;
	return left.type != ActionType::None;
}

TurnAction PhysicalAction(BehaviorMove move) {
	TurnAction action;
	action.type = ActionType::Attack;
	if (move == Move::Thrust) action.attackStyle = AttackStyle::Thrust;
	else if (move == Move::Bash) action.attackStyle = AttackStyle::Bash;
	else action.attackStyle = AttackStyle::Slash;
	return action;
}

TurnAction GuardAction(const EnemyBehaviorProfile& profile,
	const EnemyBehaviorState& state, RNG& rng) {
	TurnAction action;
	action.type = ActionType::Defend;
	if (profile.adaptsToPlayer && state.repeatedPlayerActions >= 2) {
		if (state.lastPlayerAction.type == ActionType::CastSpell) {
			action.defenseStance = DefenseStance::AntiMagic;
			return action;
		}
		if (state.lastPlayerAction.type == ActionType::Attack) {
			switch (state.lastPlayerAction.attackStyle) {
			case AttackStyle::Slash: action.defenseStance = DefenseStance::AntiSlash; break;
			case AttackStyle::Thrust: action.defenseStance = DefenseStance::AntiThrust; break;
			case AttackStyle::Bash: action.defenseStance = DefenseStance::AntiBash; break;
			}
			return action;
		}
	}
	action.defenseStance = static_cast<DefenseStance>(rng.NextInt(0, 3));
	return action;
}

TurnAction CastAction(const Enemy& enemy, RNG& rng) {
	std::vector<int> usable;
	std::vector<int> restorative;
	std::vector<int> offensive;
	const auto& spells = enemy.GetKnownSpells();
	for (std::size_t i = 0; i < spells.size(); ++i) {
		if (enemy.GetMana() < spells[i].manaCost) continue;
		usable.push_back(static_cast<int>(i));
		if (spells[i].effect != SpellEffect::Damage
			|| spells[i].name == "Soul Drain") restorative.push_back(static_cast<int>(i));
		if (spells[i].effect == SpellEffect::Damage) offensive.push_back(static_cast<int>(i));
	}
	if (usable.empty()) return PhysicalAction(Move::Slash);
	const bool wounded = enemy.GetHP() * 2 < enemy.GetMaxHP();
	const std::vector<int>& choices = wounded && !restorative.empty()
		? restorative : (!offensive.empty() ? offensive : usable);
	TurnAction action;
	action.type = ActionType::CastSpell;
	action.spellIndex = choices[static_cast<std::size_t>(rng.NextInt(0,
		static_cast<int>(choices.size()) - 1))];
	return action;
}
} // namespace

namespace EnemyBehavior {

const EnemyBehaviorProfile& Profile(EnemyArchetype archetype) {
	return Profiles[static_cast<std::size_t>(archetype)];
}

TurnAction Decide(const Enemy& enemy, EnemyBehaviorState& state, RNG& rng) {
	const EnemyBehaviorProfile& profile = Profile(enemy.GetArchetype());

	// Species follow a recognizable rhythm rather than rolling every action from
	// scratch. Irregularity occasionally skips ahead, preserving identity while
	// preventing the cadence from becoming a solved sequence.
	int rhythmIndex = state.decisionsMade % static_cast<int>(profile.rhythm.size());
	if (profile.irregularity > 0.0
		&& rng.Chance(static_cast<float>(profile.irregularity))) {
		rhythmIndex = (rhythmIndex + rng.NextInt(1,
			static_cast<int>(profile.rhythm.size()) - 1))
			% static_cast<int>(profile.rhythm.size());
	}
	++state.decisionsMade;
	const BehaviorMove move = profile.rhythm[static_cast<std::size_t>(rhythmIndex)];
	switch (move) {
	case Move::Slash:
	case Move::Thrust:
	case Move::Bash: return PhysicalAction(move);
	case Move::Defend: return GuardAction(profile, state, rng);
	case Move::Cast: return CastAction(enemy, rng);
	case Move::Hesitate: return TurnAction{};
	}
	return TurnAction{};
}

void ObservePlayerAction(EnemyBehaviorState& state, const TurnAction& action) {
	if (ActionsMatch(state.lastPlayerAction, action)) ++state.repeatedPlayerActions;
	else state.repeatedPlayerActions = 1;
	state.lastPlayerAction = action;
}

double OnHitStatusChance(const Enemy& enemy) {
	const auto& trait = Profile(enemy.GetArchetype()).onHitStatus;
	if (!trait) return 0.0;
	return std::min(trait->chanceCap,
		trait->baseChance + (enemy.GetRank() - 1) * trait->chancePerRank);
}

int OnHitStatusPotency(const Enemy& enemy) {
	const auto& trait = Profile(enemy.GetArchetype()).onHitStatus;
	if (!trait) return 0;
	return 1 + (enemy.GetRank() - 1) / std::max(1, trait->potencyRankDivisor);
}

double EarlyCommitmentChance(const Enemy& enemy, const TurnAction& action) {
	const CommitmentTendencies& tendencies = Profile(enemy.GetArchetype()).commitment;
	switch (action.type) {
	case ActionType::Attack: return tendencies.physical;
	case ActionType::CastSpell: return tendencies.spell;
	case ActionType::Defend: return tendencies.guard;
	default: return 0.0;
	}
}

bool RollEarlyCommitment(const Enemy& enemy, const TurnAction& action, RNG& rng) {
	return rng.Chance(static_cast<float>(EarlyCommitmentChance(enemy, action)));
}

bool ShouldReviseUncommittedPlan(const Enemy& enemy, RNG& rng) {
	return rng.Chance(static_cast<float>(
		Profile(enemy.GetArchetype()).commitment.uncommittedRevision));
}

bool SameAction(const TurnAction& left, const TurnAction& right) {
	return ActionsMatch(left, right);
}

} // namespace EnemyBehavior
