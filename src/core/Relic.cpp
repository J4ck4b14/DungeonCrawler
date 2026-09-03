#include "Relic.h"

#include <stdexcept>

const std::vector<RelicInfo>& AllRelics() {
	static const std::vector<RelicInfo> relics = {
		{ RelicId::BerserkersBrand, "berserkers_brand", "Berserker's Brand",
		  "+3 ATK, but -6 max HP. Rage has a price.", RelicRarity::Common, 1, 1 },
		{ RelicId::GiantsBelt, "giants_belt", "Giant's Belt",
		  "+18 max HP, but -1 ATK. Slow and steady.", RelicRarity::Common, 1, 1 },
		{ RelicId::AdrenalGland, "adrenal_gland", "Adrenal Gland",
		  "+1 SPD, but -4 max HP. Strike first, bleed later.", RelicRarity::Common, 1, 1 },
		{ RelicId::ScholarsMonocle, "scholars_monocle", "Scholar's Monocle",
		  "+2 INT (+6 max Mana, better perception), but -4 max HP.", RelicRarity::Common, 1, 1 },
		{ RelicId::VampiricFang, "vampiric_fang", "Vampiric Fang",
		  "Heal 20% of physical damage dealt.", RelicRarity::Uncommon, 1, 2 },
		{ RelicId::ThornedCarapace, "thorned_carapace", "Thorned Carapace",
		  "Enemies take 3 damage whenever they hurt you.", RelicRarity::Uncommon, 1, 2 },
		{ RelicId::LuckyCoin, "lucky_coin", "Lucky Coin",
		  "Slash critical chance doubled (15% -> 30%).", RelicRarity::Common, 1, 1 },
		{ RelicId::ExecutionersEdge, "executioners_edge", "Executioner's Edge",
		  "+50% physical damage against enemies below 30% HP.", RelicRarity::Rare, 1, 4 },
		{ RelicId::ArcaneBattery, "arcane_battery", "Arcane Battery",
		  "Spells cost 1 less mana (minimum 1).", RelicRarity::Uncommon, 1, 2 },
		{ RelicId::PhoenixFeather, "phoenix_feather", "Phoenix Feather",
		  "Death-save timing windows are 250 ms wider.", RelicRarity::Rare, 1, 4 },
		{ RelicId::HuntersLens, "hunters_lens", "Hunter's Lens",
		  "The first Inspect action in each combat is free.", RelicRarity::Common, 2, 1 },
		{ RelicId::BloodLedger, "blood_ledger", "Blood Ledger",
		  "Defeating an enemy restores 3 HP.", RelicRarity::Uncommon, 2, 2 },
		{ RelicId::RiposteSeal, "riposte_seal", "Riposte Seal",
		  "A perfect parry empowers your next damaging action by 20%.", RelicRarity::Uncommon, 3, 3 },
		{ RelicId::AegisCoil, "aegis_coil", "Aegis Coil",
		  "The first complete block each round restores 1 Mana.", RelicRarity::Uncommon, 3, 3 },
		{ RelicId::ManaPrism, "mana_prism", "Mana Prism",
		  "Exploiting an elemental weakness restores 1 Mana.", RelicRarity::Rare, 4, 5 },
		{ RelicId::LastEmber, "last_ember", "Last Ember",
		  "A successful death save restores 20% max HP.", RelicRarity::Rare, 4, 6 },
	};
	return relics;
}

const RelicInfo& GetRelicInfo(RelicId id) {
	const size_t index = static_cast<size_t>(id);
	if (index >= AllRelics().size()) throw std::out_of_range("Invalid relic id");
	return AllRelics()[index];
}

const RelicInfo* FindRelicByKey(const std::string& key) {
	for (const RelicInfo& info : AllRelics()) {
		if (key == info.key) return &info;
	}
	return nullptr;
}

const char* RelicRarityName(RelicRarity rarity) {
	switch (rarity) {
	case RelicRarity::Common: return "COMMON";
	case RelicRarity::Uncommon: return "UNCOMMON";
	case RelicRarity::Rare: return "RARE";
	}
	return "UNKNOWN";
}
