#pragma once

#include "CombatTypes.h"
#include <algorithm>

namespace CombatRules {

inline constexpr float SlashCritChance = 0.15f;
inline constexpr float LuckyCoinCritChance = 0.30f;
inline constexpr float SlashCritMultiplier = 1.5f;
inline constexpr float ThrustDamageMultiplier = 0.8f;
inline constexpr float BashMissChance = 0.15f;
inline constexpr float BashDamageMultiplier = 1.3f;
inline constexpr float BashRecoilMultiplier = 0.3f;
inline constexpr float PhysicalCounterMultiplier = 1.3f;
inline constexpr float MagicCounterMultiplier = 0.9f;
inline constexpr float WeaknessMultiplier = 1.5f;
inline constexpr float ExecutionerMultiplier = 1.5f;

constexpr bool IsPhysicalParry(DefenseStance stance, AttackStyle style) {
	return (stance == DefenseStance::AntiSlash && style == AttackStyle::Slash)
		|| (stance == DefenseStance::AntiThrust && style == AttackStyle::Thrust)
		|| (stance == DefenseStance::AntiBash && style == AttackStyle::Bash);
}

inline int EffectiveManaCost(int baseCost, bool hasArcaneBattery) {
	return hasArcaneBattery ? std::max(1, baseCost - 1) : baseCost;
}

inline int PhysicalCounterDamage(int attack) {
	return static_cast<int>(attack * PhysicalCounterMultiplier);
}

inline int MagicCounterDamage(int attack) {
	return std::max(1, static_cast<int>(attack * MagicCounterMultiplier));
}

} // namespace CombatRules
