#include "Equipment.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace {
std::size_t SlotIndex(ApparelSlot slot) { return static_cast<std::size_t>(slot); }
int ScaledRank(int rank, double multiplier) {
	return std::max(1, static_cast<int>(std::floor(rank * multiplier)));
}
double DiminishingChance(int potencyRank, double cap, double halfSaturation) {
	const double rank = static_cast<double>(std::max(EquipmentRules::MinimumRank, potencyRank));
	return cap * rank / (rank + halfSaturation);
}
} // namespace

namespace EquipmentRules {
std::string ToRomanRank(int rank) {
	rank = std::max(MinimumRank, rank);
	struct Numeral { int value; std::string_view text; };
	static constexpr Numeral numerals[] = {
		{1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"},
		{90, "XC"}, {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"},
		{5, "V"}, {4, "IV"}, {1, "I"}
	};
	std::string result;
	for (const Numeral& numeral : numerals) {
		while (rank >= numeral.value) { result += numeral.text; rank -= numeral.value; }
	}
	return result;
}

std::string_view WeaponArchetypeName(WeaponArchetype archetype) {
	switch (archetype) {
	case WeaponArchetype::Sword: return "Sword";
	case WeaponArchetype::WalkingStick: return "Walking Stick";
	case WeaponArchetype::Scimitar: return "Scimitar";
	case WeaponArchetype::Dagger: return "Dagger";
	case WeaponArchetype::Claymore: return "Claymore";
	case WeaponArchetype::Rapier: return "Rapier";
	}
	return "Weapon";
}
std::string_view EnchantmentName(EnchantmentType enchantment) {
	switch (enchantment) {
	case EnchantmentType::FireForm: return "Fire-form";
	case EnchantmentType::Drain: return "Drain";
	case EnchantmentType::IceForm: return "Ice-form";
	case EnchantmentType::SnakeTongue: return "Snake-Tongue";
	case EnchantmentType::WindForm: return "Wind-form";
	}
	return "Enchantment";
}
std::string_view ApparelFamilyName(ApparelFamily family) {
	switch (family) {
	case ApparelFamily::Knight: return "Knight";
	case ApparelFamily::Mage: return "Mage";
	case ApparelFamily::Thief: return "Thief";
	}
	return "Apparel";
}
std::string_view ApparelSlotName(ApparelSlot slot) {
	switch (slot) {
	case ApparelSlot::Head: return "Headpiece";
	case ApparelSlot::Hands: return "Gloves";
	case ApparelSlot::Torso: return "Torso";
	case ApparelSlot::Legs: return "Legwear";
	case ApparelSlot::Feet: return "Boots";
	}
	return "Apparel";
}
int MitigatePhysicalDamage(int incomingDamage, int armor) {
	if (incomingDamage <= 0) return 0;
	armor = std::max(0, armor);
	const double multiplier = 100.0 / (100.0 + 8.0 * armor);
	return std::max(1, static_cast<int>(std::ceil(incomingDamage * multiplier)));
}
} // namespace EquipmentRules

namespace EnchantmentRules {
double ProcChance(EnchantmentType type, int potencyRank) {
	switch (type) {
	case EnchantmentType::FireForm: return DiminishingChance(potencyRank, 0.60, 5.0);
	case EnchantmentType::Drain: return DiminishingChance(potencyRank, 0.50, 6.0);
	case EnchantmentType::IceForm: return DiminishingChance(potencyRank, 0.30, 8.0);
	case EnchantmentType::SnakeTongue: return DiminishingChance(potencyRank, 0.55, 5.0);
	case EnchantmentType::WindForm: return DiminishingChance(potencyRank, 0.35, 8.0);
	}
	return 0.0;
}
int BurnStacks(int potencyRank) { return 1 + (std::max(1, potencyRank) - 1) / 4; }
int PoisonPotency(int potencyRank) { return std::max(1, 1 + potencyRank / 2); }
int DrainHealing(int damageDealt) { return damageDealt > 0 ? std::max(1, damageDealt / 5) : 0; }
} // namespace EnchantmentRules

Weapon::Weapon(WeaponArchetype archetype, int weaponRank)
	: archetype_(archetype), weaponRank_(std::max(EquipmentRules::MinimumRank, weaponRank)) {}
WeaponArchetype Weapon::GetArchetype() const { return archetype_; }
int Weapon::GetWeaponRank() const { return weaponRank_; }
int Weapon::GetOverallRank() const {
	int rank = weaponRank_;
	for (const Enchantment& enchantment : enchantments_) rank += std::max(0, enchantment.potencyRank);
	return rank;
}
int Weapon::CalculateDamage(int strength, int speed, int intelligence) const {
	const double str = static_cast<double>(strength);
	const double spd = static_cast<double>(speed);
	const double intellect = static_cast<double>(intelligence);
	double damage = 1.0;
	switch (archetype_) {
	case WeaponArchetype::Sword:
		damage = 3.0 + weaponRank_ + std::max(0.0, 0.25 * str) + std::max(0.0, 0.20 * intellect); break;
	case WeaponArchetype::WalkingStick:
		damage = 3.0 + weaponRank_ + 0.10 * str; break;
	case WeaponArchetype::Scimitar:
		damage = 1.0 + weaponRank_ + std::max(0.0, 3.0 * (spd - 3.0)); break;
	case WeaponArchetype::Dagger:
		damage = 1.0 + weaponRank_ + std::max(0.0, 6.0 * (spd - 8.0)); break;
	case WeaponArchetype::Claymore:
		damage = 1.0 + weaponRank_ + std::max(0.0, 6.0 * (str - 5.0)); break;
	case WeaponArchetype::Rapier:
		damage = weaponRank_ + std::max(0.0, 2.0 * spd + 2.0 * intellect); break;
	}
	return std::max(1, static_cast<int>(std::floor(damage)));
}
const std::vector<Enchantment>& Weapon::GetEnchantments() const { return enchantments_; }
bool Weapon::AddEnchantment(Enchantment enchantment) {
	if (enchantments_.size() >= MaximumEnchantments) return false;
	const auto duplicate = std::find_if(enchantments_.begin(), enchantments_.end(),
		[&](const Enchantment& existing) { return existing.type == enchantment.type; });
	if (duplicate != enchantments_.end()) return false;
	enchantment.potencyRank = std::max(EquipmentRules::MinimumRank, enchantment.potencyRank);
	enchantments_.push_back(enchantment);
	return true;
}
void Weapon::Sharpen() { ++weaponRank_; }
std::string Weapon::GetDisplayName() const {
	std::string name = std::string(EquipmentRules::WeaponArchetypeName(archetype_))
		+ " " + EquipmentRules::ToRomanRank(weaponRank_);
	for (const Enchantment& enchantment : enchantments_) {
		name += " - " + std::string(EquipmentRules::EnchantmentName(enchantment.type))
			+ " " + EquipmentRules::ToRomanRank(enchantment.potencyRank);
	}
	return name;
}

ApparelEffects& ApparelEffects::operator+=(const ApparelEffects& other) {
	armor += other.armor; spellPower += other.spellPower;
	maxMana += other.maxMana; speed += other.speed; return *this;
}
Apparel::Apparel(ApparelFamily family, ApparelSlot slot, int itemRank)
	: family_(family), slot_(slot), itemRank_(std::max(EquipmentRules::MinimumRank, itemRank)) {}
ApparelFamily Apparel::GetFamily() const { return family_; }
ApparelSlot Apparel::GetSlot() const { return slot_; }
int Apparel::GetItemRank() const { return itemRank_; }
ApparelEffects Apparel::GetDerivedEffects() const {
	ApparelEffects effects;
	if (family_ == ApparelFamily::Knight) {
		double multiplier = 0.65;
		if (slot_ == ApparelSlot::Head) multiplier = 0.90;
		else if (slot_ == ApparelSlot::Legs) multiplier = 1.15;
		else if (slot_ == ApparelSlot::Torso) multiplier = 1.50;
		effects.armor = ScaledRank(itemRank_, multiplier);
	} else if (family_ == ApparelFamily::Mage) {
		double spellMultiplier = 0.35;
		if (slot_ == ApparelSlot::Head) spellMultiplier = 0.60;
		else if (slot_ == ApparelSlot::Hands) spellMultiplier = 0.90;
		else if (slot_ == ApparelSlot::Torso) spellMultiplier = 0.70;
		effects.spellPower = ScaledRank(itemRank_, spellMultiplier);
		if (slot_ == ApparelSlot::Torso) effects.maxMana = ScaledRank(itemRank_, 1.50);
		else if (slot_ == ApparelSlot::Head || slot_ == ApparelSlot::Hands)
			effects.maxMana = ScaledRank(itemRank_, 0.40);
	} else {
		double multiplier = 0.45;
		if (slot_ == ApparelSlot::Head || slot_ == ApparelSlot::Hands) multiplier = 0.70;
		else if (slot_ == ApparelSlot::Legs) multiplier = 1.00;
		else if (slot_ == ApparelSlot::Feet) multiplier = 1.20;
		effects.speed = std::max(1, static_cast<int>(std::floor(multiplier * std::sqrt(static_cast<double>(itemRank_)))));
	}
	return effects;
}
void Apparel::Improve() { ++itemRank_; }
std::string Apparel::GetDisplayName() const {
	return std::string(EquipmentRules::ApparelFamilyName(family_)) + " "
		+ std::string(EquipmentRules::ApparelSlotName(slot_)) + " "
		+ EquipmentRules::ToRomanRank(itemRank_);
}

const std::optional<Weapon>& EquipmentSlots::GetWeapon() const { return weapon_; }
const std::optional<Apparel>& EquipmentSlots::GetApparel(ApparelSlot slot) const { return apparel_[SlotIndex(slot)]; }
std::optional<Weapon>& EquipmentSlots::GetWeapon() { return weapon_; }
std::optional<Apparel>& EquipmentSlots::GetApparel(ApparelSlot slot) { return apparel_[SlotIndex(slot)]; }
ApparelEffects EquipmentSlots::GetTotalApparelEffects() const {
	ApparelEffects total;
	for (const std::optional<Apparel>& item : apparel_) if (item) total += item->GetDerivedEffects();
	return total;
}
bool EquipmentSlots::EquipWeapon(Weapon weapon) {
	const bool replaced = weapon_.has_value(); weapon_ = std::move(weapon); return replaced;
}
bool EquipmentSlots::EquipApparel(Apparel apparel) {
	std::optional<Apparel>& slot = apparel_[SlotIndex(apparel.GetSlot())];
	const bool replaced = slot.has_value(); slot = std::move(apparel); return replaced;
}
