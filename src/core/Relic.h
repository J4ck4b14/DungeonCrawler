// Relic.h
// -------
// Roguelike relic (boon) system.
// Relics persist for one run. The profile controls which definitions may
// appear, while minimum floor and rarity keep loot progression depth-aware.
//
// Effect hook locations:
//   Stat relics       -> Player::GrantRelic (permanent stat mutation)
//   Vampiric Fang     -> CombatSystem physical hit (heal 20% of dmg dealt)
//   Thorned Carapace  -> CombatSystem when player is hit (attacker takes 3)
//   Lucky Coin        -> CombatSystem Slash crit roll (15% -> 30%)
//   Executioner's Edge-> CombatSystem physical dmg vs targets below 30% HP
//   Arcane Battery    -> CombatSystem spell cast (mana cost -1, min 1)
//   Phoenix Feather   -> Death-save QTE (+250ms reaction window)
//   Hunter's Lens     -> CombatSystem Inspect action accounting
//   Blood Ledger      -> CombatSystem enemy defeat handling
//   Riposte/Aegis     -> CombatSystem reactive defense resolution
//   Mana Prism        -> CombatSystem elemental weakness hit
//   Last Ember        -> CombatSystem successful death save recovery

#pragma once
#include <string>
#include <vector>

enum class RelicId {
	BerserkersBrand,   // +3 ATK, -6 max HP
	GiantsBelt,        // +18 max HP, -1 ATK
	AdrenalGland,      // +1 SPD, -4 max HP
	ScholarsMonocle,   // +2 INT (+6 max Mana), -4 max HP
	VampiricFang,      // Heal 20% of physical damage you deal
	ThornedCarapace,   // Attackers take 3 damage when they hurt you
	LuckyCoin,         // Slash crit chance 15% -> 30%
	ExecutionersEdge,  // +50% physical damage vs enemies below 30% HP
	ArcaneBattery,     // Spells cost 1 less mana (minimum 1)
	PhoenixFeather,    // Death-save heartbeats are easier to match
	HuntersLens,       // First Inspect in each combat is a free action
	BloodLedger,       // Defeating an enemy restores a little HP
	RiposteSeal,       // Perfect parries empower the next attack
	AegisCoil,         // First complete block each round restores Mana
	ManaPrism,         // Exploiting a weakness restores Mana
	LastEmber,         // Successful death saves restore more HP
	COUNT
};

enum class RelicRarity {
	Common,
	Uncommon,
	Rare
};

struct RelicInfo {
	RelicId id;
	const char* key;
	const char* name;
	const char* description;
	RelicRarity rarity;
	int unlockRank;
	int minimumFloor;
};

const std::vector<RelicInfo>& AllRelics();
const RelicInfo& GetRelicInfo(RelicId id);
const RelicInfo* FindRelicByKey(const std::string& key);
const char* RelicRarityName(RelicRarity rarity);
