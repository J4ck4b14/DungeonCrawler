#pragma once

#include "combat/Spell.h"
#include "status/StatusEffect.h"

#include <optional>

struct SpellStatusRule {
	StatusType type = StatusType::Burn;
	double chance = 0.0;
	int potency = 0;
	int duration = 0;
};

namespace SpellRules {

int CalculateDamage(const Spell& spell, int strength, int speed,
	int intelligence, int casterRank = 1, int jumpIndex = 0);
int CalculateHealing(const Spell& spell, int maximumHp, int intelligence);
int RegenerationStacks(const Spell& spell, int maximumHp, int intelligence);
int DrainHealing(const Spell& spell, int damageDealt);
int DamageAgainstConventionalGuard(const Spell& spell, int damage);
std::optional<SpellStatusRule> StatusRule(const Spell& spell, int casterRank);
bool IsMultiTarget(const Spell& spell);
bool IsChain(const Spell& spell);
int AffectedTargetCount(const Spell& spell, int livingEnemyCount);
bool SuppressesRegeneration(const Spell& spell);

} // namespace SpellRules
