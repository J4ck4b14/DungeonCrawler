#pragma once

#include "Room.h"

enum class BreachOutcome {
	NoRoute,
	NoProgress,
	Damaged,
	Opened
};

struct BreachAttempt {
	BreachOutcome outcome = BreachOutcome::NoRoute;
	int force = 0;
	int remainingToughness = 0;
};

namespace BreachRules {
	int WeaponSuitabilityBonus(const char* weaponArchetype);
	int PhysicalForce(int strength, int roll, const char* weaponArchetype = nullptr);
	BreachAttempt ResolvePhysical(const HiddenWall& wall, int strength, int roll,
		const char* weaponArchetype = nullptr);
	BreachAttempt ResolveSpell(const HiddenWall& wall, int damage, SpellElement element);
}
