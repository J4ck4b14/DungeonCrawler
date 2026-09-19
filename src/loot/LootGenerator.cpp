#include "LootGenerator.h"

#include "utils/RNG.h"

#include <algorithm>
#include <array>

namespace {
constexpr std::array<WeaponArchetype, 6> WeaponTypes = {
	WeaponArchetype::Sword, WeaponArchetype::WalkingStick,
	WeaponArchetype::Scimitar, WeaponArchetype::Dagger,
	WeaponArchetype::Claymore, WeaponArchetype::Rapier
};
constexpr std::array<EnchantmentType, 5> EnchantmentTypes = {
	EnchantmentType::FireForm, EnchantmentType::Drain,
	EnchantmentType::IceForm, EnchantmentType::SnakeTongue,
	EnchantmentType::WindForm
};
constexpr std::array<ApparelFamily, 3> ApparelFamilies = {
	ApparelFamily::Knight, ApparelFamily::Mage, ApparelFamily::Thief
};
constexpr std::array<ApparelSlot, 5> ApparelSlots = {
	ApparelSlot::Head, ApparelSlot::Hands, ApparelSlot::Torso,
	ApparelSlot::Legs, ApparelSlot::Feet
};

int WeightedFirstPotency(int budget, RNG& rng) {
	int totalWeight = 0;
	for (int rank = 1; rank <= budget; ++rank) totalWeight += rank * rank;
	int roll = rng.NextInt(1, totalWeight);
	for (int rank = 1; rank <= budget; ++rank) {
		roll -= rank * rank;
		if (roll <= 0) return rank;
	}
	return budget;
}
} // namespace

namespace LootGenerator {

LootWeights CalculateCategoryWeights(const LootContext& context) {
	// Legacy has triple the influence of current character level. Keeping every
	// weight positive preserves both weak late drops and exceptional early drops.
	const double pull = std::max(0, context.legacyRank) * 3.0
		+ std::max(0, context.characterLevel - 1);
	LootWeights weights;
	weights.common = 40.0 / (1.0 + 0.012 * pull);
	weights.rare = 30.0 + 0.25 * pull;
	weights.epic = 15.0 + 0.35 * std::max(0.0, pull - 20.0);
	weights.legendary = 5.0 + 0.30 * std::max(0.0, pull - 50.0);
	return weights;
}

std::pair<int, int> OverallRankRange(LootCategory category) {
	switch (category) {
	case LootCategory::Common: return {1, 6};
	case LootCategory::Rare: return {7, 12};
	case LootCategory::Epic: return {13, 24};
	case LootCategory::Legendary: return {25, 50};
	}
	return {1, 6};
}

LootCategory CategoryForOverallRank(int overallRank) {
	if (overallRank <= 6) return LootCategory::Common;
	if (overallRank <= 12) return LootCategory::Rare;
	if (overallRank <= 24) return LootCategory::Epic;
	return LootCategory::Legendary;
}

LootCategory RollCategory(const LootContext& context, RNG& rng) {
	const LootWeights weights = CalculateCategoryWeights(context);
	const double total = weights.common + weights.rare + weights.epic + weights.legendary;
	const double roll = rng.NextFloat(0.0f, static_cast<float>(total));
	if (roll < weights.common) return LootCategory::Common;
	if (roll < weights.common + weights.rare) return LootCategory::Rare;
	if (roll < weights.common + weights.rare + weights.epic) return LootCategory::Epic;
	return LootCategory::Legendary;
}

int RollOverallRank(const LootContext& context, RNG& rng) {
	const auto [minimum, maximum] = OverallRankRange(RollCategory(context, rng));
	return rng.NextInt(minimum, maximum);
}

std::vector<int> DecomposeEnchantmentBudget(int budget, RNG& rng) {
	if (budget <= 0) return {};
	const int first = WeightedFirstPotency(budget, rng);
	if (first == budget) return {first};
	return {first, budget - first};
}

Weapon GenerateWeapon(const LootContext& context, RNG& rng) {
	const int overallRank = RollOverallRank(context, rng);
	const int weaponRank = rng.NextInt(1, overallRank);
	Weapon weapon(WeaponTypes[static_cast<std::size_t>(rng.NextInt(0,
		static_cast<int>(WeaponTypes.size()) - 1))], weaponRank);
	const std::vector<int> potencyRanks = DecomposeEnchantmentBudget(
		overallRank - weaponRank, rng);
	int firstType = -1;
	for (int potency : potencyRanks) {
		int typeIndex = rng.NextInt(0, static_cast<int>(EnchantmentTypes.size()) - 1);
		if (typeIndex == firstType) {
			typeIndex = (typeIndex + rng.NextInt(1,
				static_cast<int>(EnchantmentTypes.size()) - 1))
				% static_cast<int>(EnchantmentTypes.size());
		}
		weapon.AddEnchantment({EnchantmentTypes[static_cast<std::size_t>(typeIndex)], potency});
		if (firstType < 0) firstType = typeIndex;
	}
	return weapon;
}

Apparel GenerateApparel(const LootContext& context, RNG& rng) {
	const int rank = RollOverallRank(context, rng);
	return Apparel(
		ApparelFamilies[static_cast<std::size_t>(rng.NextInt(0,
			static_cast<int>(ApparelFamilies.size()) - 1))],
		ApparelSlots[static_cast<std::size_t>(rng.NextInt(0,
			static_cast<int>(ApparelSlots.size()) - 1))], rank);
}

GeneratedEquipment GenerateEquipment(const LootContext& context, RNG& rng) {
	if (rng.Chance(0.55f)) return GenerateWeapon(context, rng);
	return GenerateApparel(context, rng);
}

} // namespace LootGenerator
