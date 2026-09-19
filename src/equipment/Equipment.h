#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class WeaponArchetype { Sword, WalkingStick, Scimitar, Dagger, Claymore, Rapier };
enum class EnchantmentType { FireForm, Drain, IceForm, SnakeTongue, WindForm };

struct Enchantment {
	EnchantmentType type = EnchantmentType::FireForm;
	int potencyRank = 1;
};

class Weapon {
public:
	static constexpr std::size_t MaximumEnchantments = 2;
	Weapon(WeaponArchetype archetype, int weaponRank);
	WeaponArchetype GetArchetype() const;
	int GetWeaponRank() const;
	int GetOverallRank() const;
	int CalculateDamage(int strength, int speed, int intelligence) const;
	const std::vector<Enchantment>& GetEnchantments() const;
	bool AddEnchantment(Enchantment enchantment);
	void Sharpen();
	std::string GetDisplayName() const;
private:
	WeaponArchetype archetype_;
	int weaponRank_;
	std::vector<Enchantment> enchantments_;
};

enum class ApparelFamily { Knight, Mage, Thief };
enum class ApparelSlot { Head, Hands, Torso, Legs, Feet };

struct ApparelEffects {
	int armor = 0;
	int spellPower = 0;
	int maxMana = 0;
	int speed = 0;
	ApparelEffects& operator+=(const ApparelEffects& other);
};

class Apparel {
public:
	Apparel(ApparelFamily family, ApparelSlot slot, int itemRank);
	ApparelFamily GetFamily() const;
	ApparelSlot GetSlot() const;
	int GetItemRank() const;
	ApparelEffects GetDerivedEffects() const;
	void Improve();
	std::string GetDisplayName() const;
private:
	ApparelFamily family_;
	ApparelSlot slot_;
	int itemRank_;
};

class EquipmentSlots {
public:
	static constexpr std::size_t ApparelSlotCount = 5;
	const std::optional<Weapon>& GetWeapon() const;
	const std::optional<Apparel>& GetApparel(ApparelSlot slot) const;
	std::optional<Weapon>& GetWeapon();
	std::optional<Apparel>& GetApparel(ApparelSlot slot);
	ApparelEffects GetTotalApparelEffects() const;
	bool EquipWeapon(Weapon weapon);
	bool EquipApparel(Apparel apparel);
private:
	std::optional<Weapon> weapon_;
	std::array<std::optional<Apparel>, ApparelSlotCount> apparel_;
};

namespace EquipmentRules {
inline constexpr int MinimumRank = 1;
std::string ToRomanRank(int rank);
std::string_view WeaponArchetypeName(WeaponArchetype archetype);
std::string_view EnchantmentName(EnchantmentType enchantment);
std::string_view ApparelFamilyName(ApparelFamily family);
std::string_view ApparelSlotName(ApparelSlot slot);
int MitigatePhysicalDamage(int incomingDamage, int armor);
} // namespace EquipmentRules

namespace EnchantmentRules {
double ProcChance(EnchantmentType type, int potencyRank);
int BurnStacks(int potencyRank);
int PoisonPotency(int potencyRank);
int DrainHealing(int damageDealt);
} // namespace EnchantmentRules
