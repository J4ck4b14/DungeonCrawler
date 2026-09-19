#include "BreachRules.h"

#include <algorithm>
#include <string_view>

int BreachRules::WeaponSuitabilityBonus(const char* weaponArchetype) {
	if (!weaponArchetype) return 0;
	const std::string_view weapon(weaponArchetype);
	if (weapon == "Claymore" || weapon == "Axe") return 3;
	if (weapon == "Sword") return 1;
	if (weapon == "Dagger" || weapon == "Bow" || weapon == "Staff") return -1;
	return 0;
}

int BreachRules::PhysicalForce(int strength, int roll, const char* weaponArchetype) {
	return std::max(1, strength + std::clamp(roll, 1, 20) / 4
		+ WeaponSuitabilityBonus(weaponArchetype));
}

BreachAttempt BreachRules::ResolvePhysical(const HiddenWall& wall, int strength,
	int roll, const char* weaponArchetype) {
	if (!wall.exists || !wall.breakable || wall.type == WallType::Solid) return {};
	BreachAttempt result;
	result.force = PhysicalForce(strength, roll, weaponArchetype);
	if (wall.material == WallMaterial::Strange) result.force = std::max(1, result.force / 3);
	result.remainingToughness = std::max(0, wall.toughness - result.force);
	result.outcome = result.remainingToughness == 0 ? BreachOutcome::Opened : BreachOutcome::Damaged;
	return result;
}

BreachAttempt BreachRules::ResolveSpell(const HiddenWall& wall, int damage,
	SpellElement element) {
	if (!wall.exists || !wall.breakable || wall.type == WallType::Solid) return {};
	BreachAttempt result;
	result.force = std::max(1, damage);
	if (element == wall.weakness) {
		result.force = wall.material == WallMaterial::Strange
			? wall.toughness : result.force + result.force / 2;
	}
	result.remainingToughness = std::max(0, wall.toughness - result.force);
	result.outcome = result.remainingToughness == 0 ? BreachOutcome::Opened : BreachOutcome::Damaged;
	return result;
}
