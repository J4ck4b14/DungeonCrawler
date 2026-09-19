#pragma once

#include "equipment/Equipment.h"

#include <utility>
#include <variant>
#include <vector>

class RNG;

enum class LootCategory { Common, Rare, Epic, Legendary };

struct LootWeights {
	double common = 40.0;
	double rare = 30.0;
	double epic = 15.0;
	double legendary = 5.0;
};

struct LootContext {
	int legacyRank = 0;
	int characterLevel = 1;
};

using GeneratedEquipment = std::variant<Weapon, Apparel>;

namespace LootGenerator {

LootWeights CalculateCategoryWeights(const LootContext& context);
std::pair<int, int> OverallRankRange(LootCategory category);
LootCategory CategoryForOverallRank(int overallRank);
LootCategory RollCategory(const LootContext& context, RNG& rng);
int RollOverallRank(const LootContext& context, RNG& rng);

// Turns the enchantment part of an OR budget into one or two positive ranks.
// The first rank is biased high; a remainder becomes the second enchantment.
std::vector<int> DecomposeEnchantmentBudget(int budget, RNG& rng);

Weapon GenerateWeapon(const LootContext& context, RNG& rng);
Apparel GenerateApparel(const LootContext& context, RNG& rng);
GeneratedEquipment GenerateEquipment(const LootContext& context, RNG& rng);

} // namespace LootGenerator
