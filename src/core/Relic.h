// Relic.h
// -------
// Roguelike relic (boon) system.
// After clearing each floor, the player is offered a choice of 3 random
// relics they don't own yet -- take one, or walk away. Relics persist for
// the entire run and die with the hero. Most carry a tradeoff, so every
// pick shapes the build.
//
// Effect hook locations:
//   Stat relics       -> Player::GrantRelic (permanent stat mutation)
//   Vampiric Fang     -> CombatSystem physical hit (heal 20% of dmg dealt)
//   Thorned Carapace  -> CombatSystem when player is hit (attacker takes 3)
//   Lucky Coin        -> CombatSystem Slash crit roll (15% -> 30%)
//   Executioner's Edge-> CombatSystem physical dmg vs targets below 30% HP
//   Arcane Battery    -> CombatSystem spell cast (mana cost -1, min 1)
//   Phoenix Feather   -> Death-save QTE (+250ms reaction window)

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
	COUNT
};

struct RelicInfo {
	RelicId id;
	const char* name;
	const char* description;
};

inline const std::vector<RelicInfo>& AllRelics() {
	static const std::vector<RelicInfo> relics = {
		{ RelicId::BerserkersBrand,  "Berserker's Brand",
		  "+3 ATK, but -6 max HP. Rage has a price." },
		{ RelicId::GiantsBelt,       "Giant's Belt",
		  "+18 max HP, but -1 ATK. Slow and steady." },
		{ RelicId::AdrenalGland,     "Adrenal Gland",
		  "+1 SPD, but -4 max HP. Strike first, bleed later." },
		{ RelicId::ScholarsMonocle,  "Scholar's Monocle",
		  "+2 INT (+6 max Mana, better perception), but -4 max HP." },
		{ RelicId::VampiricFang,     "Vampiric Fang",
		  "Heal 20% of the physical damage you deal." },
		{ RelicId::ThornedCarapace,  "Thorned Carapace",
		  "Enemies take 3 damage whenever they hurt you." },
		{ RelicId::LuckyCoin,        "Lucky Coin",
		  "Slash critical chance doubled (15% -> 30%)." },
		{ RelicId::ExecutionersEdge, "Executioner's Edge",
		  "+50% physical damage against enemies below 30% HP." },
		{ RelicId::ArcaneBattery,    "Arcane Battery",
		  "Spells cost 1 less mana (minimum 1)." },
		{ RelicId::PhoenixFeather,   "Phoenix Feather",
		  "Your heart beats slower at death's door (easier death saves)." },
	};
	return relics;
}

inline const RelicInfo& GetRelicInfo(RelicId id) {
	return AllRelics()[static_cast<size_t>(id)];
}
