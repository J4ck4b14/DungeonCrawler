#include "combat/CombatRules.h"
#include "combat/DefenseRules.h"
#include "combat/EncounterRules.h"
#include "combat/Spell.h"
#include "combat/SpellRules.h"
#include "core/RelicRules.h"
#include "core/EnemyDescriptions.h"
#include "dungeon/DungeonRules.h"
#include "dungeon/DungeonTopology.h"
#include "dungeon/BreachRules.h"
#include "dungeon/Perception.h"
#include "dungeon/RestSite.h"
#include "dungeon/Room.h"
#include "equipment/Equipment.h"
#include "entities/Enemy.h"
#include "entities/EnemyBehavior.h"
#include "entities/EnemyDefinitions.h"
#include "entities/EnemyFactory.h"
#include "entities/Player.h"
#include "loot/LootGenerator.h"
#include "presentation/CombatDisplay.h"
#include "presentation/CombatMenu.h"
#include "audio/MusicSystem.h"
#include "presentation/EnemyIntent.h"
#include "progression/PlayerProfile.h"
#include "utils/RNG.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Expect(bool condition, const std::string& message) {
	if (!condition) {
		std::cerr << "FAILED: " << message << '\n';
		++failures;
	}
}

void TestCombatRules() {
	Expect(CombatRules::IsPhysicalParry(DefenseStance::AntiSlash, AttackStyle::Slash),
		"AntiSlash parries Slash");
	Expect(!CombatRules::IsPhysicalParry(DefenseStance::AntiMagic, AttackStyle::Slash),
		"AntiMagic does not parry physical attacks");
	Expect(CombatRules::PhysicalCounterDamage(10) == 13,
		"physical counter keeps its 1.3x truncating calculation");
	Expect(CombatRules::MagicCounterDamage(1) == 1,
		"magic counter retains its minimum damage");
}

void TestEntityAndRelicRules() {
	Stats stats;
	stats.hp = 1;
	stats.strength = 10;
	stats.speed = 4;
	stats.intelligence = 3;
	stats.RecalculateDerived();
	Player player("Test Hero", stats);
	Expect(player.GetStrength() == 10,
		"Entity exposes the renamed Strength stat without changing its value");

	player.SetDefending(true);
	player.ReceiveDamage(5);
	Expect(player.GetHP() == player.GetMaxHP() - 2,
		"defense halves damage using integer truncation");
	const int strengthBeforeRelic = player.GetStrength();
	player.GrantRelic(RelicId::BerserkersBrand);
	Expect(player.GetStrength() == strengthBeforeRelic + 3,
		"Strength is used by stat-changing relics");
	Expect(player.ActionsPerRound(2) == 2, "double speed grants two actions");
	Expect(player.ActionsPerRound(3) == 1, "less than double speed grants one action");

	Spell spell{"Test", SpellElement::Arcane, SpellTarget::Enemy, 3, 1, 0};
	Expect(player.GetEffectiveManaCost(spell) == 3, "base spell cost is unchanged");
	player.GrantRelic(RelicId::ArcaneBattery);
	Expect(player.GetEffectiveManaCost(spell) == 2, "Arcane Battery discounts spell cost");
	spell.manaCost = 1;
	Expect(player.GetEffectiveManaCost(spell) == 1, "Arcane Battery keeps minimum cost at one");

	player.ApplyPowerBuff(25, 2);
	Expect(player.ConsumePowerBuff(10) == 12,
		"Empower applies its percentage bonus with integer truncation");
	Expect(player.ConsumePowerBuff(10) == 12,
		"Empower remains active for its second damaging action");
	Expect(player.ConsumePowerBuff(10) == 10
		&& player.GetPowerBuff().remainingHits == 0,
		"Empower expires after the configured number of damaging actions");
	player.ApplyPowerBuff(25, 2);
	player.ApplyPowerBuff(20, 1);
	Expect(player.GetPowerBuff().percentBonus == 25
		&& player.GetPowerBuff().remainingHits == 2,
		"a smaller riposte buff cannot weaken an active Empower spell");
	player.ConsumePowerBuff(10);
	player.ConsumePowerBuff(10);
	const Spell* empower = FindSpell("Empower");
	Expect(empower != nullptr && empower->effect == SpellEffect::Empower
		&& empower->target == SpellTarget::Self && empower->duration == 2,
		"Empower is defined as a two-charge self buff");
}

const EnemyDefinition& Definition(EnemyArchetype archetype) {
	const auto& definitions = GetEnemyDefinitions();
	const auto found = std::find_if(definitions.begin(), definitions.end(),
		[&](const EnemyDefinition& definition) {
			return definition.archetype == archetype;
		});
	return *found;
}

void TestEquipmentFoundations() {
	constexpr int strength = 10;
	constexpr int speed = 10;
	constexpr int intelligence = 5;
	Expect(Weapon(WeaponArchetype::Sword, 4).CalculateDamage(strength, speed, intelligence) == 10,
		"Sword uses WR plus mixed STR and INT scaling");
	Expect(Weapon(WeaponArchetype::WalkingStick, 4).CalculateDamage(strength, speed, intelligence) == 8,
		"Walking Stick uses its light STR scaling");
	Expect(Weapon(WeaponArchetype::Scimitar, 4).CalculateDamage(strength, speed, intelligence) == 26,
		"Scimitar scales from Speed above three");
	Expect(Weapon(WeaponArchetype::Dagger, 4).CalculateDamage(strength, speed, intelligence) == 17,
		"Dagger scales from Speed above eight");
	Expect(Weapon(WeaponArchetype::Claymore, 4).CalculateDamage(strength, speed, intelligence) == 35,
		"Claymore scales from Strength above five");
	Expect(Weapon(WeaponArchetype::Rapier, 4).CalculateDamage(strength, speed, intelligence) == 34,
		"Rapier combines Speed and Intelligence");

	Weapon sword(WeaponArchetype::Sword, 3);
	Expect(sword.AddEnchantment({EnchantmentType::FireForm, 2})
		&& !sword.AddEnchantment({EnchantmentType::FireForm, 1})
		&& sword.AddEnchantment({EnchantmentType::SnakeTongue, 1}),
		"weapons reject duplicate enchantments and accept two distinct ones");
	const int oldDamage = sword.CalculateDamage(strength, speed, intelligence);
	const int oldOverallRank = sword.GetOverallRank();
	sword.Sharpen();
	Expect(sword.GetWeaponRank() == 4 && sword.GetOverallRank() == oldOverallRank + 1
		&& sword.CalculateDamage(strength, speed, intelligence) == oldDamage + 1,
		"Sharpen raises WR, OR, and damage by one");
	Expect(sword.GetDisplayName().find("OR") == std::string::npos,
		"normal weapon names hide internal overall rank");

	EquipmentSlots slots;
	Expect(!slots.EquipWeapon(sword), "first weapon fills the empty slot");
	Expect(slots.EquipWeapon(Weapon(WeaponArchetype::Dagger, 7))
		&& slots.GetWeapon()->GetArchetype() == WeaponArchetype::Dagger,
		"a new weapon permanently replaces the old weapon");

	Apparel knightHead(ApparelFamily::Knight, ApparelSlot::Head, 10);
	Apparel knightHands(ApparelFamily::Knight, ApparelSlot::Hands, 10);
	Apparel knightTorso(ApparelFamily::Knight, ApparelSlot::Torso, 10);
	Apparel knightLegs(ApparelFamily::Knight, ApparelSlot::Legs, 10);
	Apparel knightFeet(ApparelFamily::Knight, ApparelSlot::Feet, 10);
	Expect(knightTorso.GetDerivedEffects().armor > knightLegs.GetDerivedEffects().armor
		&& knightLegs.GetDerivedEffects().armor > knightHead.GetDerivedEffects().armor
		&& knightHead.GetDerivedEffects().armor > knightHands.GetDerivedEffects().armor
		&& knightHands.GetDerivedEffects().armor == knightFeet.GetDerivedEffects().armor,
		"Knight armor follows torso, legs, head, hands/feet weighting");
	Apparel mageHands(ApparelFamily::Mage, ApparelSlot::Hands, 10);
	Apparel mageHead(ApparelFamily::Mage, ApparelSlot::Head, 10);
	Apparel mageTorso(ApparelFamily::Mage, ApparelSlot::Torso, 10);
	Expect(mageHands.GetDerivedEffects().spellPower > mageHead.GetDerivedEffects().spellPower
		&& mageTorso.GetDerivedEffects().maxMana > mageHead.GetDerivedEffects().maxMana,
		"Mage hands emphasize spell output and torso emphasizes mana");
	Apparel thiefFeet(ApparelFamily::Thief, ApparelSlot::Feet, 25);
	Apparel thiefTorso(ApparelFamily::Thief, ApparelSlot::Torso, 25);
	Expect(thiefFeet.GetDerivedEffects().speed > thiefTorso.GetDerivedEffects().speed
		&& thiefFeet.GetDerivedEffects().speed < thiefFeet.GetItemRank(),
		"Thief mobility favors feet and grows sublinearly");

	Expect(EquipmentRules::MitigatePhysicalDamage(20, 0) == 20
		&& EquipmentRules::MitigatePhysicalDamage(20, 10) < 20
		&& EquipmentRules::MitigatePhysicalDamage(20, 100000) == 1,
		"armor diminishes physical damage without reducing it to zero");
	for (EnchantmentType type : {EnchantmentType::FireForm, EnchantmentType::Drain,
		EnchantmentType::IceForm, EnchantmentType::SnakeTongue, EnchantmentType::WindForm}) {
		Expect(EnchantmentRules::ProcChance(type, 1) > 0.0
			&& EnchantmentRules::ProcChance(type, 100000) < 1.0
			&& EnchantmentRules::ProcChance(type, 10) > EnchantmentRules::ProcChance(type, 1),
			"enchantment curves are positive, increasing, and capped below certainty");
	}

	Stats stats;
	stats.hp = 2;
	stats.strength = 5;
	stats.speed = 4;
	stats.intelligence = 3;
	stats.RecalculateDerived();
	Player player("Equipped Hero", stats);
	const int baseMana = player.GetMaxMana();
	const int baseSpeed = player.GetSpeed();
	player.EquipApparel(Apparel(ApparelFamily::Mage, ApparelSlot::Torso, 8));
	player.EquipApparel(Apparel(ApparelFamily::Thief, ApparelSlot::Feet, 9));
	Expect(player.GetMaxMana() > baseMana && player.GetSpeed() > baseSpeed,
		"equipped Mage and Thief apparel feed their derived bonuses into player rules");
	player.EquipWeapon(Weapon(WeaponArchetype::Rapier, 3));
	Expect(player.GetWeaponDamage() == player.GetEquipment().GetWeapon()->CalculateDamage(
		player.GetStrength(), player.GetSpeed(), player.GetIntelligence()),
		"player physical damage uses the equipped weapon formula");
}

void TestStatusEffectFoundations() {
	Stats stats;
	stats.hp = 10;
	stats.maxHp = 70;
	stats.strength = 5;
	stats.speed = 3;
	stats.intelligence = 1;
	stats.maxMana = 3;

	Player player("Afflicted Hero", stats);
	player.GetStatuses().ApplyPoison(2);
	player.GetStatuses().ApplyPoison(3);
	StatusTurnResult poisonTick = player.ProcessStatusTurn();
	Expect(poisonTick.poisonDamage == 5
		&& player.GetStatuses().Get(StatusType::Poison).potency == 4,
		"poison stacks, deals its current amount, then decrements by one");
	poisonTick = player.ProcessStatusTurn();
	Expect(poisonTick.poisonDamage == 4
		&& player.GetStatuses().Get(StatusType::Poison).potency == 3,
		"poison continues its deterministic countdown on player turns");

	Enemy enemy("Status Target", stats);
	enemy.GetStatuses().ApplyBurn(2, 2);
	StatusTurnResult burnTick = enemy.ProcessStatusTurn();
	Expect(burnTick.burnDamage == 2 * StatusTuning::BurnDamagePerStack
		&& enemy.GetStatuses().Has(StatusType::Burn),
		"burn stacks deal centralized passive damage to enemy entities");
	burnTick = enemy.ProcessStatusTurn();
	Expect(burnTick.burnDamage == 2 * StatusTuning::BurnDamagePerStack
		&& !enemy.GetStatuses().Has(StatusType::Burn),
		"burn expires after its configured duration");

	enemy.GetStatuses().ApplyBleed(5);
	StatusTurnResult bleedTick = enemy.ProcessStatusTurn();
	Expect(bleedTick.bleedDamage == 0
		&& enemy.GetStatuses().Get(StatusType::Bleed).potency == 5,
		"Bleed builds as pressure without ordinary damage-over-time ticks");
	bleedTick = enemy.ProcessStatusTurn();
	Expect(bleedTick.bleedDamage == 0
		&& enemy.GetStatuses().Get(StatusType::Bleed).potency == 3,
		"Bleed charge drains during turns without fresh aggression");
	enemy.GetStatuses().ApplyBleed(7);
	bleedTick = enemy.ProcessStatusTurn();
	Expect(bleedTick.bleedDamage == 10
		&& !enemy.GetStatuses().Has(StatusType::Bleed),
		"a filled Bleed meter bursts once for about ten normal charge damage and empties");

	enemy.GetStatuses().ApplyFreeze(2);
	Expect(enemy.ProcessStatusTurn().skipAction
		&& enemy.ProcessStatusTurn().skipAction
		&& !enemy.ProcessStatusTurn().skipAction,
		"freeze consumes exactly the configured number of actions");

	enemy.ReceiveDamage(12);
	enemy.GetStatuses().ApplyRegeneration(2, 2);
	const int hpBeforeRegeneration = enemy.GetHP();
	StatusTurnResult regenerationTick = enemy.ProcessStatusTurn();
	Expect(regenerationTick.regenerationHealing
		== 2 * StatusTuning::RegenerationHealingPerStack
		&& enemy.GetHP() > hpBeforeRegeneration,
		"regeneration heals an afflicted enemy over time");
	regenerationTick = enemy.ProcessStatusTurn(true);
	Expect(regenerationTick.regenerationHealing == 0
		&& !enemy.GetStatuses().Has(StatusType::Regeneration),
		"regeneration can be temporarily suppressed by a future Fire interaction");
}

void TestDungeonRules() {
	const RoomContentWeights defaults =
		DungeonRules::CalculateRoomContentWeights(1, 1.0f, 1.0f);
	Expect(defaults.combat == 42 && defaults.chest == 15
		&& defaults.trap == 9 && defaults.rest == 11 && defaults.empty == 23,
		"default floor-one room weights are preserved");
	Expect(DungeonRules::TotalRoomContentWeight(defaults) == 100,
		"floor-one explicit room weights sum to the intended total");
	Expect(DungeonRules::SelectRoomContent(defaults, 42) == RoomContent::Combat
		&& DungeonRules::SelectRoomContent(defaults, 43) == RoomContent::Chest
		&& DungeonRules::SelectRoomContent(defaults, 58) == RoomContent::Trap
		&& DungeonRules::SelectRoomContent(defaults, 67) == RoomContent::Rest
		&& DungeonRules::SelectRoomContent(defaults, 78) == RoomContent::Empty,
		"weighted room selection uses cumulative category boundaries");

	const RoomContentWeights noTraps =
		DungeonRules::CalculateRoomContentWeights(1, 1.0f, 0.0f);
	Expect(noTraps.trap == 0 && noTraps.rest == 100,
		"zero trap multiplier is finite and disables traps");
	const RoomContentWeights lateNoTraps =
		DungeonRules::CalculateRoomContentWeights(12, 1.0f, 0.0f);
	Expect(lateNoTraps.rest == 2,
		"zero trap multiplier preserves the late-floor rest minimum");

	double firstCombatRate = 0.0, deepCombatRate = 0.0;
	for (int floor : {1, 5, 10, 20, 30, 40, 50}) {
		const RoomContentWeights weights = DungeonRules::CalculateRoomContentWeights(
			floor, 1.0f, 1.0f);
		std::array<int, 5> counts{};
		RNG rng(static_cast<unsigned>(3100 + floor));
		constexpr int Samples = 30000;
		for (int i = 0; i < Samples; ++i) {
			const RoomContent content = DungeonRules::RollRoomContent(weights, rng);
			int index = 4;
			if (content == RoomContent::Combat) index = 0;
			else if (content == RoomContent::Chest) index = 1;
			else if (content == RoomContent::Trap) index = 2;
			else if (content == RoomContent::Rest) index = 3;
			++counts[index];
		}
		Expect(std::all_of(counts.begin(), counts.end(), [](int count) { return count > 0; }),
			"all ordinary room categories remain reachable at floor " + std::to_string(floor));
		const double combatRate = static_cast<double>(counts[0]) / Samples;
		if (floor == 1) firstCombatRate = combatRate;
		if (floor == 50) deepCombatRate = combatRate;
	}
	Expect(deepCombatRate > firstCombatRate,
		"combat becomes more common with depth without eliminating systemic variety");
}

void TestEncounterRules() {
	Expect(EncounterRules::MultiEnemyChancePercent(1) == 0,
		"floor one has no multi-enemy encounters");
	Expect(EncounterRules::DetermineEnemyCount(1, 1) == 1,
		"floor one always produces a single enemy");
	Expect(EncounterRules::MultiEnemyChancePercent(2) == 8,
		"floor two starts with an eight-percent group chance");
	Expect(EncounterRules::DetermineEnemyCount(2, 8) == 2,
		"the floor-two group threshold produces a pair");
	Expect(EncounterRules::DetermineEnemyCount(2, 9) == 1,
		"a roll above the floor-two threshold remains solo");
	Expect(EncounterRules::ThreeEnemyChancePercent(5) == 0,
		"three-enemy groups cannot appear before floor six");
	Expect(EncounterRules::DetermineEnemyCount(6, 2) == 3,
		"floor six introduces a small three-enemy chance");
	Expect(EncounterRules::DetermineEnemyCount(6, 3) == 2,
		"floor six still produces pairs above the triple threshold");
	Expect(EncounterRules::MultiEnemyChancePercent(100) == 28,
		"multi-enemy chance is capped");
	Expect(EncounterRules::ThreeEnemyChancePercent(100) == 6,
		"three-enemy chance is capped");
}

void TestRoomAndDefinitions() {
	Room room;
	room.SetHiddenWall(Direction::East, 8, WallMaterial::Wood, SpellElement::Fire);
	room.ClearHiddenWall(Direction::East);
	Expect(!room.HasHiddenExit(Direction::East)
		&& room.GetHiddenToughness(Direction::East) == 0,
		"clearing a hidden exit clears its toughness");
	Expect(room.GetHiddenWall(Direction::East).material == WallMaterial::None,
		"clearing a hidden exit clears its material");
	const std::string sealedWall = Perception::DescribeWall(
		Direction::North, true, false, 20, 4, 30);
	Expect(sealedWall.find("hollow") != std::string::npos
		&& sealedWall.find("breakable") == std::string::npos
		&& sealedWall.find("force it") == std::string::npos,
		"an unusual but unbreakable wall never promises a route");
	const std::string weakWall = Perception::DescribeWall(
		Direction::North, true, true, 8, 4, 12);
	Expect(weakWall.find("breakable") != std::string::npos
		&& weakWall.find("force it") != std::string::npos,
		"survey language exposes a genuinely breakable wall when skill permits");

	const auto& enemies = GetEnemyDefinitions();
	Expect(enemies.size() == 16, "all enemy definitions remain available");
	Expect(enemies.front().name == "Slime" && enemies.back().name == "Dragon",
		"enemy definition ordering is preserved");
	for (const EnemyDefinition& enemy : enemies) {
		Expect(enemy.minimumLevel > 0, enemy.name + " has a valid minimum level");
		Expect(enemy.minHp <= enemy.maxHp && enemy.minStrength <= enemy.maxStrength
			&& enemy.minSpeed <= enemy.maxSpeed
			&& enemy.minIntelligence <= enemy.maxIntelligence,
			enemy.name + " has ordered stat ranges");
		for (const std::string& spellName : enemy.spellNames) {
			Expect(FindSpell(spellName) != nullptr,
				enemy.name + " references known spell " + spellName);
		}
	}
}

void TestSeededRng() {
	RNG first(12345);
	RNG second(12345);
	for (int i = 0; i < 10; ++i) {
		Expect(first.NextInt(1, 100) == second.NextInt(1, 100),
			"equal RNG seeds produce equal sequences");
	}
}

void TestLootGeneration() {
	Expect(LootGenerator::OverallRankRange(LootCategory::Common) == std::pair<int, int>{1, 6}
		&& LootGenerator::OverallRankRange(LootCategory::Rare) == std::pair<int, int>{7, 12}
		&& LootGenerator::OverallRankRange(LootCategory::Epic) == std::pair<int, int>{13, 24}
		&& LootGenerator::OverallRankRange(LootCategory::Legendary) == std::pair<int, int>{25, 50},
		"loot category boundaries match the specified OR ranges");
	Expect(LootGenerator::CategoryForOverallRank(6) == LootCategory::Common
		&& LootGenerator::CategoryForOverallRank(7) == LootCategory::Rare
		&& LootGenerator::CategoryForOverallRank(12) == LootCategory::Rare
		&& LootGenerator::CategoryForOverallRank(13) == LootCategory::Epic
		&& LootGenerator::CategoryForOverallRank(24) == LootCategory::Epic
		&& LootGenerator::CategoryForOverallRank(25) == LootCategory::Legendary,
		"category lookup preserves every boundary transition");

	RNG splitRng(991);
	int firstOne = 0, firstTwo = 0, firstThree = 0;
	for (int i = 0; i < 10000; ++i) {
		const std::vector<int> parts = LootGenerator::DecomposeEnchantmentBudget(3, splitRng);
		Expect(!parts.empty() && parts.size() <= 2
			&& parts[0] > 0 && (parts.size() == 1 || parts[1] > 0)
			&& parts[0] + (parts.size() == 2 ? parts[1] : 0) == 3,
			"enchantment decomposition exactly preserves a positive budget");
		if (parts[0] == 1) ++firstOne;
		else if (parts[0] == 2) ++firstTwo;
		else if (parts[0] == 3) ++firstThree;
	}
	Expect(firstThree > firstTwo && firstTwo > firstOne
		&& firstOne > 400 && firstOne < 1000
		&& firstTwo > 2200 && firstTwo < 3600
		&& firstThree > 5600 && firstThree < 7200,
		"a budget of three follows the intended roughly 5/30/65 high-first bias");

	RNG freshRng(123456);
	RNG veteranRng(123456);
	double freshTotal = 0.0;
	double veteranTotal = 0.0;
	bool freshFoundLegendary = false;
	bool veteranFoundCommon = false;
	constexpr int samples = 20000;
	for (int i = 0; i < samples; ++i) {
		const int fresh = LootGenerator::RollOverallRank({1, 1}, freshRng);
		const int veteran = LootGenerator::RollOverallRank({40, 1}, veteranRng);
		freshTotal += fresh;
		veteranTotal += veteran;
		freshFoundLegendary = freshFoundLegendary || fresh >= 25;
		veteranFoundCommon = veteranFoundCommon || veteran <= 6;
	}
	Expect(veteranTotal / samples > freshTotal / samples + 5.0,
		"high Legacy creates a substantial upward statistical loot shift");
	Expect(freshFoundLegendary, "a fresh profile retains a small chance of exceptional loot");
	Expect(veteranFoundCommon, "a high-Legacy profile can still receive low-rank loot");

	const LootWeights legacyStep = LootGenerator::CalculateCategoryWeights({1, 1});
	const LootWeights levelStep = LootGenerator::CalculateCategoryWeights({0, 2});
	Expect(legacyStep.rare - 30.0 > (levelStep.rare - 30.0) * 2.0,
		"one Legacy rank has more than twice one current-level rank of loot influence");

	RNG weaponRng(44);
	for (int i = 0; i < 500; ++i) {
		const Weapon weapon = LootGenerator::GenerateWeapon({17, 4}, weaponRng);
		Expect(weapon.GetOverallRank() >= 1 && weapon.GetOverallRank() <= 50,
			"generated weapon ranks stay within the rolled OR limits");
		if (weapon.GetEnchantments().size() == 2) {
			Expect(weapon.GetEnchantments()[0].type != weapon.GetEnchantments()[1].type,
				"generated weapons never duplicate enchantment identities");
		}
	}
}

void TestRestSites() {
	Stats stats;
	stats.hp = 3;
	stats.strength = 5;
	stats.speed = 4;
	stats.intelligence = 3;
	stats.RecalculateDerived();
	Player player("Rest Tester", stats);
	player.EquipWeapon(Weapon(WeaponArchetype::Sword, 2));
	player.EquipApparel(Apparel(ApparelFamily::Knight, ApparelSlot::Torso, 3));

	RestSite sharpenSite;
	const int oldWeaponRank = player.GetEquipment().GetWeapon()->GetWeaponRank();
	Expect(sharpenSite.Use(player, RestAction::SharpenWeapon, 2)
		&& sharpenSite.IsConsumed()
		&& player.GetEquipment().GetWeapon()->GetWeaponRank() == oldWeaponRank + 1,
		"Sharpen consumes one site and permanently adds one WR");
	Expect(!sharpenSite.Use(player, RestAction::TrainStrength, 2),
		"a rest site rejects every action after its first valid use");

	RestSite improveSite;
	const int oldApparelRank = player.GetEquipment().GetApparel(ApparelSlot::Torso)->GetItemRank();
	Expect(improveSite.Use(player, RestAction::ImproveApparel, 2, ApparelSlot::Torso)
		&& player.GetEquipment().GetApparel(ApparelSlot::Torso)->GetItemRank() == oldApparelRank + 1,
		"Improve Apparel consumes a site and adds one item rank");

	const int oldStrength = player.GetStrength();
	for (int i = 0; i < 6; ++i) Expect(player.TrainStat(1),
		"valid training remains available without a global cap");
	Expect(player.GetTrainingPoints() == 6 && player.GetStrength() == oldStrength + 6
		&& player.CanTrain(),
		"training can exceed the removed three-use cap");
}

void TestEnemyRanksAndSpeciesScaling() {
	for (const EnemyDefinition& definition : GetEnemyDefinitions()) {
		Expect(EnemyFactory::CalculateRank(definition, 1, -100) == 1
			&& EnemyFactory::CalculateRank(definition, 100, 100) == 50,
			definition.name + " specimen Rank remains clamped to 1-50");
		Expect(!EnemyBehavior::Profile(definition.archetype).rhythm.empty(),
			definition.name + " has an authored behavior rhythm");
	}
	RNG invalidFloorRng(7000);
	const Enemy invalidFloorEnemy = EnemyFactory::CreateEnemy(0, invalidFloorRng);
	Expect(invalidFloorEnemy.GetRank() >= 1 && invalidFloorEnemy.GetRank() <= 50,
		"enemy generation safely treats invalid low floors as floor one");

	RNG firstSeed(7001);
	RNG secondSeed(7001);
	const Enemy first = EnemyFactory::CreateEnemy(8, firstSeed);
	const Enemy second = EnemyFactory::CreateEnemy(8, secondSeed);
	Expect(first.GetName() == second.GetName() && first.GetRank() == second.GetRank()
		&& first.GetMaxHP() == second.GetMaxHP()
		&& first.GetStrength() == second.GetStrength(),
		"enemy generation depends on depth, species, and RNG rather than player level");

	auto lowAndHigh = [](EnemyArchetype archetype) {
		RNG lowRng(91);
		RNG highRng(91);
		return std::pair<Enemy, Enemy>{
			EnemyFactory::CreateEnemy(Definition(archetype), 1, lowRng),
			EnemyFactory::CreateEnemy(Definition(archetype), 25, highRng)};
	};

	const auto [lowGiant, highGiant] = lowAndHigh(EnemyArchetype::Giant);
	Expect(highGiant.GetMaxHP() - lowGiant.GetMaxHP()
		> highGiant.GetSpeed() - lowGiant.GetSpeed()
		&& highGiant.GetSpeed() <= 3,
		"Giant Rank strongly grows HP while preserving its slow readable identity");
	const auto [lowMage, highMage] = lowAndHigh(EnemyArchetype::DarkMage);
	Expect(highMage.GetIntelligence() - lowMage.GetIntelligence()
		> highMage.GetStrength() - lowMage.GetStrength()
		&& highMage.GetMaxMana() - lowMage.GetMaxMana() > 20,
		"Dark Mage Rank growth is primarily magical");
	const auto [lowWerewolf, highWerewolf] = lowAndHigh(EnemyArchetype::Werewolf);
	Expect(highWerewolf.GetSpeed() - lowWerewolf.GetSpeed()
		> highWerewolf.GetStrength() - lowWerewolf.GetStrength(),
		"Werewolf Rank scaling favors speed and burst pressure");
	const auto [lowDemon, highDemon] = lowAndHigh(EnemyArchetype::Demon);
	Expect(EnemyBehavior::OnHitStatusPotency(highDemon)
		> EnemyBehavior::OnHitStatusPotency(lowDemon)
		&& EnemyBehavior::OnHitStatusChance(highDemon)
		> EnemyBehavior::OnHitStatusChance(lowDemon),
		"Demon Burn danger grows in both potency and bounded reliability");
	Expect(highDemon.GetXPReward() > lowDemon.GetXPReward(),
		"higher-Rank specimens grant more species-weighted XP");
}

void TestEnemyBehaviorAndStatuses() {
	Stats stats;
	stats.maxHp = 100;
	stats.strength = 8;
	stats.speed = 3;
	stats.intelligence = 5;
	stats.maxMana = 50;

	RNG skeletonRng(1);
	Enemy skeleton("Skeleton", stats, {}, 1, SpellElement::Arcane,
		1, EnemyArchetype::Skeleton);
	Expect(skeleton.DecideTurn(skeletonRng).attackStyle == AttackStyle::Slash
		&& skeleton.DecideTurn(skeletonRng).attackStyle == AttackStyle::Slash
		&& skeleton.DecideTurn(skeletonRng).type == ActionType::Defend,
		"Skeleton follows a rigid, learnable slash-slash-guard rhythm");

	RNG orcRng(2);
	Enemy orc("Orc", stats, {}, 1, SpellElement::Fire, 10, EnemyArchetype::Orc);
	Expect(orc.DecideTurn(orcRng).attackStyle == AttackStyle::Bash
		&& orc.DecideTurn(orcRng).type == ActionType::None,
		"Orc exposes a recovery window after its committed heavy attack");

	const Spell* frost = FindSpell("Frost Bolt");
	Enemy bandit("Bandit", stats, frost ? std::vector<Spell>{*frost} : std::vector<Spell>{},
		1, SpellElement::Lightning, 10, EnemyArchetype::Bandit);
	RNG banditRng(3);
	bandit.DecideTurn(banditRng);
	TurnAction repeatedSlash;
	repeatedSlash.type = ActionType::Attack;
	repeatedSlash.attackStyle = AttackStyle::Slash;
	bandit.ObservePlayerAction(repeatedSlash);
	bandit.ObservePlayerAction(repeatedSlash);
	const TurnAction adaptiveGuard = bandit.DecideTurn(banditRng);
	Expect(adaptiveGuard.type == ActionType::Defend
		&& adaptiveGuard.defenseStance == DefenseStance::AntiSlash,
		"Bandit adapts its authored guard turn to a repeated player habit");

	RNG trollRng(17);
	Enemy troll = EnemyFactory::CreateEnemy(Definition(EnemyArchetype::Troll), 12, trollRng);
	Expect(troll.GetStatuses().Has(StatusType::Regeneration),
		"Troll specimens begin with Rank-scaled regeneration");
	troll.ReceiveDamage(20);
	troll.SuppressRegeneration(2);
	const int suppressedHp = troll.GetHP();
	const StatusTurnResult suppressed = troll.ProcessStatusTurn(troll.IsRegenerationSuppressed());
	troll.AdvanceRegenerationSuppression();
	Expect(suppressed.regenerationHealing == 0 && troll.GetHP() == suppressedHp
		&& troll.IsRegenerationSuppressed(),
		"Fire-style suppression prevents Troll regeneration temporarily");
}

void TestSpellIdentities() {
	const Spell& fireball = *FindSpell("Fireball");
	const Spell& inferno = *FindSpell("Inferno");
	const Spell& flameLance = *FindSpell("Flame Lance");
	const Spell& frostBolt = *FindSpell("Frost Bolt");
	const Spell& blizzard = *FindSpell("Blizzard");
	const Spell& iceShard = *FindSpell("Ice Shard");
	const Spell& spark = *FindSpell("Spark");
	const Spell& thunderbolt = *FindSpell("Thunderbolt");
	const Spell& chain = *FindSpell("Chain Lightning");
	const Spell& shadowBolt = *FindSpell("Shadow Bolt");
	const Spell& voidBlast = *FindSpell("Void Blast");
	const Spell& soulDrain = *FindSpell("Soul Drain");
	const Spell& corruptingBreath = *FindSpell("Corrupting Breath");
	const Spell& magicMissile = *FindSpell("Magic Missile");
	const Spell& arcaneBurst = *FindSpell("Arcane Burst");
	const Spell& heal = *FindSpell("Heal");
	const Spell& greaterHeal = *FindSpell("Greater Heal");
	const Spell& rejuvenation = *FindSpell("Rejuvenation");

	Expect(SpellRules::StatusRule(fireball, 1)->type == StatusType::Burn
		&& SpellRules::StatusRule(inferno, 30)->potency
		> SpellRules::StatusRule(fireball, 30)->potency,
		"Fireball can Burn while Inferno applies stronger Rank-scaled Burn");
	Expect(SpellRules::IsMultiTarget(inferno)
		&& SpellRules::AffectedTargetCount(inferno, 3) == 3,
		"Inferno targets every living enemy");
	Expect(SpellRules::CalculateDamage(flameLance, 12, 3, 5)
		> SpellRules::CalculateDamage(flameLance, 2, 3, 5),
		"Flame Lance has a partial Strength contribution");
	Expect(SpellRules::StatusRule(frostBolt, 1)->type == StatusType::Freeze
		&& SpellRules::IsMultiTarget(blizzard)
		&& SpellRules::AffectedTargetCount(blizzard, 4) == 4
		&& SpellRules::CalculateDamage(blizzard, 5, 5, 5)
		< SpellRules::CalculateDamage(frostBolt, 5, 5, 5),
		"Blizzard trades single-target damage for encounter-wide Freeze pressure");
	const int shardDamage = SpellRules::CalculateDamage(iceShard, 5, 5, 6);
	Expect(SpellRules::DamageAgainstConventionalGuard(iceShard, shardDamage)
		> shardDamage / 2,
		"Ice Shard partially bypasses conventional guard mitigation");
	Expect(SpellRules::CalculateDamage(spark, 4, 10, 4)
		> SpellRules::CalculateDamage(spark, 4, 2, 4)
		&& SpellRules::StatusRule(thunderbolt, 10)->type == StatusType::Freeze,
		"Spark uses Speed while Thunderbolt carries disruption");
	Expect(SpellRules::IsChain(chain)
		&& SpellRules::AffectedTargetCount(chain, 3) == 3
		&& SpellRules::CalculateDamage(chain, 5, 5, 8, 1, 0)
		> SpellRules::CalculateDamage(chain, 5, 5, 8, 1, 1)
		&& SpellRules::CalculateDamage(chain, 5, 5, 8, 1, 1)
		> SpellRules::CalculateDamage(chain, 5, 5, 8, 1, 2),
		"Chain Lightning reaches all targets with decreasing jump damage");
	const int shadowDamage = SpellRules::CalculateDamage(shadowBolt, 5, 5, 7);
	Expect(SpellRules::DamageAgainstConventionalGuard(shadowBolt, shadowDamage)
		> shadowDamage / 2
		&& SpellRules::CalculateDamage(voidBlast, 5, 5, 7) > shadowDamage,
		"Shadow Bolt pierces conventional defense while Void Blast is the heavy strike");
	Expect(SpellRules::DrainHealing(soulDrain, 20) == 8,
		"Soul Drain restores forty percent of actual damage dealt");
	Expect(SpellRules::IsMultiTarget(corruptingBreath)
		&& SpellRules::StatusRule(corruptingBreath, 30)->type == StatusType::Poison,
		"Corrupting Breath gives Dragons a reachable multi-target Poison family");
	Expect(SpellRules::CalculateDamage(magicMissile, 5, 5, 4)
		< SpellRules::CalculateDamage(voidBlast, 5, 5, 4)
		&& SpellRules::AffectedTargetCount(arcaneBurst, 3) == 3,
		"Magic Missile remains cheap and focused while Arcane Burst hits groups");
	Expect(SpellRules::CalculateHealing(greaterHeal, 50, 6)
		> SpellRules::CalculateHealing(heal, 50, 6)
		&& SpellRules::RegenerationStacks(rejuvenation, 80, 6)
		> SpellRules::RegenerationStacks(rejuvenation, 30, 6),
		"Greater Heal is stronger and Rejuvenation scales with Health over time");
	Expect(SpellRules::SuppressesRegeneration(fireball)
		&& !SpellRules::SuppressesRegeneration(frostBolt),
		"damaging Fire spells suppress regeneration");

	const auto dragonBurn = SpellRules::StatusRule(inferno, 30);
	const auto dragonFreeze = SpellRules::StatusRule(blizzard, 30);
	const auto dragonPoison = SpellRules::StatusRule(corruptingBreath, 30);
	Expect(dragonBurn && dragonFreeze && dragonPoison
		&& dragonBurn->type != dragonFreeze->type
		&& dragonPoison->type == StatusType::Poison,
		"Dragon's authored repertoire spans Burn, Freeze, and Poison families");
}

void TestEnemyIntentPresentation() {
	Stats stats;
	stats.maxHp = 20;
	stats.strength = 4;
	stats.speed = 2;
	stats.intelligence = 3;
	stats.maxMana = 9;
	Spell fireball{"Fireball", SpellElement::Fire, SpellTarget::Enemy, 3, 6, 2};
	Enemy enemy("Test Witch", stats, {fireball}, 1, SpellElement::Shadow,
		10, EnemyArchetype::Witch);

	TurnAction slash;
	slash.type = ActionType::Attack;
	slash.attackStyle = AttackStyle::Slash;
	TurnAction cast;
	cast.type = ActionType::CastSpell;
	cast.spellIndex = 0;

	Expect(EnemyIntent::DetermineClarity(enemy, slash,
		EnemyKnowledge::None, 0, true) == IntentClarity::Alert,
		"an unknown enemy exposes only its broad action at low Intelligence");
	Expect(EnemyIntent::DetermineClarity(enemy, slash,
		EnemyKnowledge::None, 4, true) == IntentClarity::Hinted,
		"Intelligence improves the read without automatically naming the move");
	Expect(EnemyIntent::DetermineClarity(enemy, slash,
		EnemyKnowledge::Approximate, 0, true) == IntentClarity::Hinted,
		"prior bestiary knowledge contributes to telegraph quality");
	Expect(EnemyIntent::DetermineClarity(enemy, cast,
		EnemyKnowledge::Full, 0, true) == IntentClarity::Exact,
		"excellent knowledge can identify a genuinely committed casting cadence");
	Expect(EnemyIntent::DetermineClarity(enemy, cast,
		EnemyKnowledge::Full, 30, false) != IntentClarity::Exact,
		"even exceptional knowledge cannot make an uncommitted tell certain");

	Enemy giant("Giant", stats, {}, 1, SpellElement::Lightning,
		10, EnemyArchetype::Giant);
	Enemy bandit("Bandit", stats, {}, 1, SpellElement::Lightning,
		10, EnemyArchetype::Bandit);
	Expect(EnemyIntent::DetermineClarity(giant, slash,
		EnemyKnowledge::None, 0, true)
		> EnemyIntent::DetermineClarity(bandit, slash,
			EnemyKnowledge::None, 0, true),
		"species readability makes a Giant clearer than a deceptive Bandit");
	Expect(EnemyIntent::DetermineClarity(bandit, slash,
		EnemyKnowledge::Full, 30, false) != IntentClarity::Exact,
		"full knowledge and high INT do not make an uncommitted Bandit exact");
	Expect(EnemyIntent::DetermineClarity(giant, slash,
		EnemyKnowledge::Approximate, 4, true) == IntentClarity::Exact,
		"a genuinely committed Giant windup can become exact with reasonable knowledge");
	Expect(EnemyIntent::DetermineClarity(giant, slash,
		EnemyKnowledge::None, 0, true) != IntentClarity::Exact,
		"commitment alone does not grant exact understanding to an uninformed character");
	Expect(EnemyBehavior::EarlyCommitmentChance(bandit, slash)
		< EnemyBehavior::EarlyCommitmentChance(giant, slash)
		&& EnemyBehavior::EarlyCommitmentChance(bandit, slash) < 1.0,
		"planning and commitment are separate, data-driven species properties");
	RNG commitmentRng(2026);
	bool sawUncommittedBandit = false, sawCommittedGiant = false;
	for (int i = 0; i < 100; ++i) {
		sawUncommittedBandit = sawUncommittedBandit
			|| !EnemyBehavior::RollEarlyCommitment(bandit, slash, commitmentRng);
		sawCommittedGiant = sawCommittedGiant
			|| EnemyBehavior::RollEarlyCommitment(giant, slash, commitmentRng);
	}
	Expect(sawUncommittedBandit && sawCommittedGiant,
		"internal plans do not universally become committed tells");

	const std::string veiled = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Veiled);
	const std::string hinted = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Hinted);
	const std::string clear = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Clear);
	Expect(veiled.find("SLASH") == std::string::npos,
		"veiled intent does not expose the action label");
	Expect(hinted.find("physical") != std::string::npos,
		"hinted Slash intent communicates an attack category");
	Expect(clear.find("sweeping") != std::string::npos
		&& clear.find("SLASH") == std::string::npos,
		"a very good tell predicts the motif without presenting a certain answer");
	Expect(EnemyIntent::Describe(enemy, cast, IntentClarity::Exact)
		.find("COMMITTED: Fireball") != std::string::npos,
		"exact spell intent names the committed spell");

	Expect(CombatDisplay::MakeMeter(5, 10, 10) == "[#####-----]",
		"combat meters represent health proportionally");
	Expect(CombatDisplay::MakeMeter(-2, 10, 4) == "[----]",
		"combat meters clamp negative values safely");
}

void TestMultiEnemyPresentationAndTargeting() {
	Stats playerStats;
	playerStats.maxHp = 24;
	playerStats.strength = 5;
	playerStats.speed = 4;
	playerStats.intelligence = 3;
	playerStats.maxMana = 8;
	Player player("Test Hero", playerStats);

	Stats enemyStats;
	enemyStats.maxHp = 10;
	enemyStats.strength = 3;
	enemyStats.speed = 2;
	enemyStats.intelligence = 1;
	enemyStats.maxMana = 0;
	std::vector<Enemy> enemies;
	enemies.emplace_back("Slime", enemyStats);
	enemies.emplace_back("Goblin", enemyStats);

	std::ostringstream displayOutput;
	std::streambuf* originalOutput = std::cout.rdbuf(displayOutput.rdbuf());
	CombatDisplay::PrintEncounterIntro(enemies);
	TurnAction firstIntent;
	firstIntent.type = ActionType::Attack;
	TurnAction secondIntent;
	secondIntent.type = ActionType::Defend;
	CombatDisplay::PrintRoundHeader(1, player, enemies,
		{EnemyKnowledge::None, EnemyKnowledge::Partial},
		{false, false}, {-1, 0, 1}, {firstIntent, secondIntent}, {true, true},
		{false, true});
	std::cout.rdbuf(originalOutput);
	const std::string rendered = displayOutput.str();
	Expect(rendered.find("AMBUSH! 2 enemies") != std::string::npos,
		"group encounters have a distinct introduction");
	Expect(rendered.find("[E1] Slime") != std::string::npos
		&& rendered.find("[E2] Goblin") != std::string::npos,
		"group display gives each enemy a stable label");
	Expect(rendered.find("TURN ORDER: Test Hero > E1 > E2") != std::string::npos,
		"group display shows the round initiative order");
	Expect(rendered.find("INTENT:") != std::string::npos,
		"enemy intent is rendered inside the hostile panel");

	std::istringstream targetInput("2\n");
	std::ostringstream targetOutput;
	std::streambuf* originalInput = std::cin.rdbuf(targetInput.rdbuf());
	originalOutput = std::cout.rdbuf(targetOutput.rdbuf());
	const int selected = CombatMenu::ChooseTarget(enemies, "Choose a target:");
	std::cin.rdbuf(originalInput);
	std::cin.clear();
	std::cout.rdbuf(originalOutput);
	Expect(selected == 1, "target selection maps the menu choice to E2");

	enemies.front().ReceiveDamage(enemies.front().GetMaxHP());
	Expect(CombatMenu::ChooseTarget(enemies, "Choose a target:") == 1,
		"target selection skips defeated enemies and auto-selects the survivor");
}

void TestCancelableCombatMenus() {
	Stats stats;
	stats.maxHp = 20;
	stats.strength = 4;
	stats.speed = 3;
	stats.intelligence = 1;
	stats.maxMana = 3;
	Player player("Test Hero", stats);

	std::istringstream input("1\n0\n2\n");
	std::ostringstream output;
	std::streambuf* originalInput = std::cin.rdbuf(input.rdbuf());
	std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
	const TurnAction action = CombatMenu::ChooseAction(player);
	std::cin.rdbuf(originalInput);
	std::cin.clear();
	std::cout.rdbuf(originalOutput);

	Expect(action.type == ActionType::Defend,
		"backing out of physical attack returns to the action menu");
}

void TestResolvedRoomMemory() {
	Stats stats;
	stats.maxHp = 20;
	stats.strength = 4;
	stats.speed = 3;
	stats.intelligence = 1;
	stats.maxMana = 3;
	Player player("Test Hero", stats);

	std::vector<std::vector<Room>> grid(2, std::vector<Room>(2));
	Room& current = grid[0][0];
	current.x = 0;
	current.y = 0;
	current.exists = true;
	current.visited = true;
	current.contentResolved = true;
	current.outcome = RoomOutcome::EmptySearched;
	current.perceptionUsed = true;
	current.SetExit(Direction::East, true);
	current.hints.push_back({Direction::East,
		"To the East, an untouched chest waits.", RoomContent::Chest, true});

	Room& east = grid[0][1];
	east.x = 1;
	east.y = 0;
	east.exists = true;
	east.content = RoomContent::Chest;
	east.visited = true;
	east.contentResolved = true;
	east.outcome = RoomOutcome::ChestOpened;

	std::ostringstream output;
	std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
	Perception::PerceiveFromRoom(current, grid, 2, player);
	std::cout.rdbuf(originalOutput);
	const std::string remembered = output.str();
	Expect(remembered.find("already searched the empty chamber") != std::string::npos,
		"environment checks remember the current resolved room");
	Expect(remembered.find("chest you opened stands empty") != std::string::npos,
		"recalled surveys replace stale chest hints with known outcomes");
	Expect(remembered.find("untouched chest") == std::string::npos,
		"resolved rooms do not replay stale perception text");
}

void TestMusicControls() {
	MusicSystem::SetVolume(-20);
	Expect(MusicSystem::GetVolume() == 0,
		"music volume clamps at zero");
	MusicSystem::SetVolume(140);
	Expect(MusicSystem::GetVolume() == 100,
		"music volume clamps at one hundred");
	MusicSystem::SetMuted(false);
	MusicSystem::ToggleMuted();
	Expect(MusicSystem::IsMuted(),
		"music mute can be toggled independently of volume");
	MusicSystem::SetMuted(false);
	MusicSystem::SetVolume(70);
}

void TestLegacyProgressionAndRelicPools() {
	Expect(AllRelics().size() == static_cast<size_t>(RelicId::COUNT)
		&& FindRelicByKey("riposte_seal") != nullptr,
		"the relic catalogue remains complete and exposes stable save keys");
	PlayerProfile profile;
	Expect(profile.GetLegacyRank() == 0
		&& profile.GetUnlockedRelics().size() == 6,
		"a new profile starts at rank zero with six distinct relics unlocked");
	Expect(!profile.IsRelicUnlocked(RelicId::HuntersLens),
		"new progression relics begin locked");
	for (int rank = 0; rank <= PlayerProfile::MaximumLegacyRank; ++rank) {
		std::ostringstream save;
		save << "profile_version=2\nlegacy_xp=" << PlayerProfile::XPRequiredForRank(rank)
			<< "\nruns=0\ndeaths=0\nescapes=0\nhighest_floor=0\ntotal_kills=0\n"
			<< "music_volume=70\nmusic_muted=0\nunlocked_relics=\n";
		PlayerProfile atRank;
		Expect(PlayerProfile::Deserialize(save.str(), atRank),
			"a generated profile loads at each Legacy milestone");
		Expect(atRank.GetLegacyRank() == rank,
			"Legacy rank threshold is exact at rank " + std::to_string(rank));
		Expect(atRank.GetUnlockedRelics().size()
			== static_cast<size_t>(6 + rank / 5),
			"one relic unlocks at each five-rank milestone");
		if (rank < PlayerProfile::MaximumLegacyRank) {
			Expect(PlayerProfile::XPRequiredForRank(rank + 1)
				> PlayerProfile::XPRequiredForRank(rank),
				"Legacy XP curve remains strictly increasing");
		}
	}

	CompletedRun run;
	run.floorsCleared = 4;
	run.highestFloorReached = 5;
	run.enemiesDefeated = 10;
	const LegacyReward reward = profile.CompleteRun(run);
	Expect(reward.xpEarned == 210 && profile.GetLegacyRank() == 2,
		"a substantial floor-five run reaches Legacy rank two on the new curve");
	Expect(reward.newlyUnlockedRelics.empty(),
		"non-milestone ranks do not unlock extra relics");

	const std::string saved = profile.Serialize();
	PlayerProfile loaded;
	std::string parseError;
	Expect(PlayerProfile::Deserialize(saved, loaded, &parseError),
		"a serialized legacy profile can be loaded");
	Expect(loaded.GetLegacyXP() == profile.GetLegacyXP()
		&& loaded.GetRunCount() == 1
		&& loaded.GetHighestFloor() == 5
		&& loaded.GetUnlockedRelics().size() == 6,
		"legacy profile round-tripping preserves progression and unlocks");

	const std::string oldSave =
		"profile_version=1\nlegacy_xp=200\nruns=7\ndeaths=5\nescapes=2\n"
		"highest_floor=8\ntotal_kills=44\nmusic_volume=31\nmusic_muted=1\n"
		"unlocked_relics=lucky_coin\n";
	PlayerProfile migrated;
	Expect(PlayerProfile::Deserialize(oldSave, migrated, &parseError)
		&& migrated.GetLegacyRank() == 3 && migrated.GetRunCount() == 7
		&& migrated.GetDeathCount() == 5 && migrated.GetEscapeCount() == 2
		&& migrated.GetHighestFloor() == 8 && migrated.GetTotalKills() == 44
		&& migrated.GetMusicVolume() == 31 && migrated.IsMusicMuted(),
		"version-one saves migrate progression, settings, and statistics without corruption");

	Expect(!RelicRules::IsAvailable(RelicId::ExecutionersEdge, 34, 3)
		&& RelicRules::IsAvailable(RelicId::ExecutionersEdge, 35, 4),
		"rare original relics enter the run pool only at their minimum depth");
	Expect(!RelicRules::IsAvailable(RelicId::AegisCoil, 29, 8)
		&& RelicRules::IsAvailable(RelicId::AegisCoil, 30, 3),
		"legacy and floor requirements both gate progression relics");
	const std::vector<RelicId> eligible = RelicRules::BuildEligiblePool(
		0, 1, {RelicId::LuckyCoin});
	Expect(std::find(eligible.begin(), eligible.end(), RelicId::LuckyCoin) == eligible.end()
		&& std::find(eligible.begin(), eligible.end(), RelicId::BerserkersBrand) != eligible.end(),
		"relic offers exclude owned relics without removing other eligible choices");
}

void TestReactiveDefenseRules() {
	Expect(DefenseRules::GradeTiming(-70, 220, 70) == DefenseCueGrade::Perfect
		&& DefenseRules::GradeTiming(70, 220, 70) == DefenseCueGrade::Perfect
		&& DefenseRules::GradeTiming(-71, 220, 70) == DefenseCueGrade::Block
		&& DefenseRules::GradeTiming(71, 220, 70) == DefenseCueGrade::Block,
		"visual timing bands and input grading share exact symmetric boundaries");
	Expect(DefenseRules::GradeCue('W', 'w', 25, 220, 70)
		== DefenseCueGrade::Perfect,
		"defense input is case-insensitive inside the perfect window");
	Expect(DefenseRules::GradeCue('A', 'A', 140, 220, 70)
		== DefenseCueGrade::Block,
		"correct input in the broad timing window produces a block");
	Expect(DefenseRules::GradeCue('A', 'D', 0, 220, 70)
		== DefenseCueGrade::Miss
		&& DefenseRules::GradeCue('A', 'A', 221, 220, 70)
		== DefenseCueGrade::Miss,
		"wrong and out-of-window inputs break the guard");
	Expect(DefenseRules::ResolveSequence(
		{DefenseCueGrade::Perfect, DefenseCueGrade::Perfect})
		== DefenseResult::PerfectParry,
		"every cue must be perfect for a perfect parry");
	Expect(DefenseRules::ResolveSequence(
		{DefenseCueGrade::Perfect, DefenseCueGrade::Block})
		== DefenseResult::Block,
		"a complete mixed-quality sequence blocks without countering");
	Expect(DefenseRules::ResolveSequence(
		{DefenseCueGrade::Perfect, DefenseCueGrade::Miss})
		== DefenseResult::GuardBreak,
		"one missed cue makes the attack deal full damage");
	Expect(DefenseRules::DamageAfterDefense(9, DefenseResult::GuardBreak) == 9
		&& DefenseRules::DamageAfterDefense(9, DefenseResult::Block) == 4
		&& DefenseRules::DamageAfterDefense(9, DefenseResult::PerfectParry) == 0,
		"guard break, block, and perfect parry resolve to full, half, and zero damage");

	TurnAction slash;
	slash.type = ActionType::Attack;
	slash.attackStyle = AttackStyle::Slash;
	const DefenseChallenge slowSlash = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Giant, 1, 2, 25, 1, 0);
	const DefenseChallenge fastSlash = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Werewolf, 50, 12, 8, 1, 0);
	Expect(DefenseRules::IsValidChallenge(slowSlash)
		&& slowSlash.cues.front().notes.size() == 3,
		"Giant defense uses a valid slow three-lane cluster");
	Expect(fastSlash.cues[0].fallDurationMs < slowSlash.cues[0].fallDurationMs
		&& fastSlash.blockRadiusMs < slowSlash.blockRadiusMs,
		"fast high-Rank enemies increase pressure without erasing species patterns");

	DefenseCue chord;
	chord.fallDurationMs = 1000;
	chord.notes = {{'A', {'A'}, 0}, {'S', {'S'}, 0}};
	const auto chordGrades = DefenseRules::GradeCueInputs(chord,
		{{'A', 1000}, {'S', 1060}}, 180, 50, 65);
	Expect(chordGrades.size() == 2
		&& chordGrades[0] == DefenseCueGrade::Perfect
		&& chordGrades[1] == DefenseCueGrade::Perfect,
		"near-simultaneous chord presses share the configured grace window");

	DefenseCue staggered;
	staggered.fallDurationMs = 900;
	staggered.notes = {{'W', {'W'}, 0}, {'D', {'D'}, 120}};
	const auto staggeredGrades = DefenseRules::GradeCueInputs(staggered,
		{{'W', 900}, {'D', 1020}}, 180, 50, 65);
	Expect(DefenseRules::ResolveSequence(staggeredGrades)
		== DefenseResult::PerfectParry,
		"staggered chord notes are graded against their individual arrivals");

	bool sawA = false, sawW = false, sawS = false, sawD = false;
	bool sawMotion = false, sawLate = false, sawFlicker = false, sawSparse = false;
	for (const EnemyDefinition& definition : GetEnemyDefinitions()) {
		const DefenseChallenge low = DefenseRules::BuildChallenge(slash, nullptr,
			definition.archetype, 1, 4, 8, 4, 0);
		const DefenseChallenge high = DefenseRules::BuildChallenge(slash, nullptr,
			definition.archetype, 50, 9, 15, 4, 2);
		Expect(DefenseRules::IsValidChallenge(low)
			&& DefenseRules::IsValidChallenge(high),
			definition.name + " always produces a valid four-lane pattern");
		Expect(high.cues.size() >= low.cues.size(),
			definition.name + " Rank scaling adds motifs instead of speed spam");
		for (int rank = 1; rank <= 50; ++rank) {
			const DefenseChallenge everyRank = DefenseRules::BuildChallenge(
				slash, nullptr, definition.archetype, rank,
				1 + rank / 6, 4 + rank / 4, 4, rank % 4);
			Expect(DefenseRules::IsValidChallenge(everyRank),
				definition.name + " has a valid pattern at Rank "
				+ std::to_string(rank));
		}
		for (const DefenseCue& cue : high.cues) {
			for (const DefenseNote& note : cue.notes) {
				for (char lane : note.lanePath) {
					sawA = sawA || lane == 'A'; sawW = sawW || lane == 'W';
					sawS = sawS || lane == 'S'; sawD = sawD || lane == 'D';
				}
				sawMotion = sawMotion || note.lanePath.size() > 1;
				sawLate = sawLate || note.visibility == DefenseCueVisibility::Late;
				sawFlicker = sawFlicker || note.visibility == DefenseCueVisibility::Flicker;
				sawSparse = sawSparse || note.visibility == DefenseCueVisibility::Sparse;
			}
		}
	}
	Expect(sawA && sawW && sawS && sawD,
		"authored patterns exercise all four A/W/S/D lanes");
	Expect(sawMotion && sawLate && sawFlicker && sawSparse,
		"pattern vocabulary includes motion, late tells, flicker, and sparse groups");

	const DefenseChallenge witch = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Witch, 40, 5, 7, 5, 0);
	const DefenseChallenge darkMage = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::DarkMage, 25, 5, 7, 5, 0);
	const DefenseChallenge werewolf = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Werewolf, 25, 9, 10, 5, 0);
	const DefenseChallenge rat = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Rat, 1, 5, 3, 5, 0);
	const DefenseChallenge dragon = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Dragon, 50, 9, 18, 5, 0);
	const DefenseChallenge goblin = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Goblin, 25, 7, 8, 5, 0);
	Expect(witch.patternLabel.find("Serpentine") != std::string::npos
		&& witch.cues.front().notes.front().lanePath.size() >= 5,
		"Witch cues move horizontally in a serpentine path before committing");
	Expect(darkMage.patternLabel.find("Sparse") != std::string::npos
		&& darkMage.cues.front().notes.size() >= 2,
		"Dark Mage cues arrive as sparse discrete groups");
	Expect(werewolf.patternLabel.find("burst") != std::string::npos
		&& werewolf.cues.size() >= 5,
		"Werewolf patterns preserve their irregular rapid-burst identity");
	Expect(rat.cues.front().notes.size() == 1 && rat.cues.size() > 1,
		"patterns support both isolated single cues and longer sequences");
	Expect(dragon.patternLabel.find("apex") != std::string::npos
		&& dragon.cues.front().notes.front().lanePath.size() > 1
		&& dragon.cues.back().notes.size() == 4,
		"high-Rank Dragon patterns combine authored movement and chord motifs");
	Expect(std::any_of(goblin.cues.begin(), goblin.cues.end(),
		[](const DefenseCue& cue) {
			return std::any_of(cue.notes.begin(), cue.notes.end(),
				[](const DefenseNote& note) { return note.decoy; });
		}), "Goblin patterns include false preparation before commitment");

	Expect(DefenseRules::SpeedBlockBonusMs(100000)
		<= DefenseTuning::MaximumSpeedBlockBonusMs
		&& DefenseRules::SpeedBlockBonusMs(24) - DefenseRules::SpeedBlockBonusMs(12)
		< DefenseRules::SpeedBlockBonusMs(12),
		"player Speed widens only the block window with capped diminishing returns");
	const DefenseChallenge quickPlayer = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Skeleton, 20, 5, 8, 50, 0);
	const DefenseChallenge slowPlayer = DefenseRules::BuildChallenge(slash, nullptr,
		EnemyArchetype::Skeleton, 20, 5, 8, 1, 0);
	Expect(quickPlayer.blockRadiusMs > slowPlayer.blockRadiusMs
		&& quickPlayer.perfectRadiusMs == slowPlayer.perfectRadiusMs,
		"Speed modestly helps blocking while perfect timing remains player-skill based");
}

void TestPersistentBestiary() {
	Stats stats;
	stats.maxHp = 24; stats.strength = 7; stats.speed = 6;
	stats.intelligence = 9; stats.maxMana = 12;
	Spell venom{"Venom Bolt", SpellElement::Shadow, SpellTarget::Enemy, 3, 5, 1};
	Enemy spider("Spider", stats, {venom}, 12, SpellElement::Fire,
		12, EnemyArchetype::Spider);
	PlayerProfile profile;
	Bestiary& bestiary = profile.GetBestiary();
	Expect(bestiary.RecordEncounter(spider, EnemyKnowledge::Approximate, 6),
		"the first species encounter creates a persistent field record");
	bestiary.ImproveKnowledge(spider, EnemyKnowledge::Full, 14);
	const BestiaryEntry* inferredOnly = bestiary.GetEntry("Spider");
	Expect(inferredOnly && inferredOnly->knownSpells.empty()
		&& inferredOnly->knownStatusThreats.empty(),
		"inspection knowledge does not fabricate unwitnessed spells or statuses");
	bestiary.ImproveKnowledge(spider, EnemyKnowledge::None, 0);
	bestiary.RecordKill("Spider");
	bestiary.RecordWeaknessDiscovered("Spider");
	bestiary.RecordSpellObserved("Spider", "Venom Bolt");
	bestiary.RecordStatusObserved("Spider", StatusType::Poison);
	bestiary.RecordBehaviorObserved("Spider", "Skittering lunge observed");
	const BestiaryEntry* before = bestiary.GetEntry("Spider");
	Expect(before && before->bestKnowledge == EnemyKnowledge::Full
		&& before->weaknessDiscovered && before->defeatedCount == 1,
		"Bestiary knowledge and discoveries only improve");

	PlayerProfile loaded;
	std::string error;
	Expect(PlayerProfile::Deserialize(profile.Serialize(), loaded, &error),
		"a profile containing Bestiary records round-trips");
	const BestiaryEntry* after = loaded.GetBestiary().GetEntry("Spider");
	Expect(after && after->bestKnowledge == EnemyKnowledge::Full
		&& after->knownSpells.contains("Venom Bolt")
		&& after->knownStatusThreats.contains(StatusType::Poison)
		&& after->behaviorDiscoveries.contains("Skittering lunge observed")
		&& after->highestEnemyRankObserved == 12,
		"Bestiary knowledge, observations, Rank, and behavior persist across runs");
	TurnAction attack;
	attack.type = ActionType::Attack;
	attack.attackStyle = AttackStyle::Thrust;
	Expect(after && EnemyIntent::DetermineClarity(spider, attack,
		after->bestKnowledge, 0, true) > EnemyIntent::DetermineClarity(
			spider, attack, EnemyKnowledge::None, 0, true),
		"loaded persistent knowledge feeds telegraph quality independently of current INT");
	for (const EnemyDefinition& definition : GetEnemyDefinitions()) {
		const std::string low = EnemyDescriptions::GetDescription(definition.name, 0);
		const std::string medium = EnemyDescriptions::GetDescription(definition.name, 1);
		const std::string high = EnemyDescriptions::GetDescription(definition.name, 2);
		const std::string expert = EnemyDescriptions::GetDescription(definition.name, 3);
		Expect(!low.empty() && low != medium && medium != high && high != expert,
			definition.name + " has four distinct, useful field-note layers");
	}

	Bestiary repeated;
	for (int i = 0; i < 6; ++i) repeated.RecordEncounter(spider, EnemyKnowledge::None, 0);
	Expect(repeated.GetKnowledge("Spider") == EnemyKnowledge::Partial,
		"repeated encounters build useful knowledge even without a perfect inspection");
}

void TestDungeonTopologyAndBreaching() {
	int floorsWithDeadEnds = 0, floorsWithLoops = 0;
	double rooms = 0.0, deadEnds = 0.0, junctions = 0.0, loops = 0.0, routes = 0.0;
	for (unsigned seed = 1; seed <= 1000; ++seed) {
		RNG rng(seed);
		GeneratedFloorTopology floor = DungeonTopology::Generate(1 + seed % 20, rng);
		Expect(DungeonTopology::MandatorySpacesConnected(floor),
			"seeded topology keeps every mandatory room connected");
		Expect(floor.metrics.shortestEntranceToStairs > 0,
			"seeded topology always provides a valid entrance-to-stairs route");
		Expect(floor.metrics.passageCount < floor.metrics.roomCount * 2,
			"floor topology avoids apartment-like excessive connectivity");
		floorsWithDeadEnds += floor.metrics.deadEndCount >= 2;
		floorsWithLoops += floor.metrics.loopCount > 0;
		rooms += floor.metrics.roomCount; deadEnds += floor.metrics.deadEndCount;
		junctions += floor.metrics.junctionCount; loops += floor.metrics.loopCount;
		routes += floor.metrics.shortestEntranceToStairs;
	}
	Expect(floorsWithDeadEnds >= 700,
		"meaningful dead ends appear on most seeded floors");
	Expect(floorsWithLoops >= 500 && floorsWithLoops < 950,
		"loops appear often but not on every generated floor");
	std::cout << "Topology aggregate (1000 seeds): avg rooms=" << rooms / 1000.0
		<< ", dead ends=" << deadEnds / 1000.0
		<< ", junctions=" << junctions / 1000.0
		<< ", loops=" << loops / 1000.0
		<< ", entrance-stairs=" << routes / 1000.0 << '\n';

	RNG mapRng(73);
	GeneratedFloorTopology map = DungeonTopology::Generate(8, mapRng);
	map.grid[map.entranceY][map.entranceX].visited = true;
	const std::string unknownMap = DungeonTopology::RenderKnowledge(
		map, map.entranceX, map.entranceY);
	int knownMarkers = static_cast<int>(std::count_if(unknownMap.begin(), unknownMap.end(),
		[](char c) { return c == '@' || c == 'o' || c == '.' || c == 'V'; }));
	Expect(knownMarkers == 1,
		"the map does not expose generator geometry behind unknown walls or passages");

	bool testedWall = false;
	for (int y = 0; y < map.size && !testedWall; ++y) for (int x = 0; x < map.size && !testedWall; ++x) {
		for (int d = 0; d < 4; ++d) {
			Direction direction = static_cast<Direction>(d);
			if (!map.grid[y][x].IsWallBreakable(direction)) continue;
			map.grid[y][x].visited = true;
			map.grid[y][x].SetWallKnowledge(direction, WallKnowledge::Suspected);
			const std::string suspected = DungeonTopology::RenderKnowledge(map, x, y);
			const std::string geometry = suspected.substr(0, suspected.find("Suspicious walls:"));
			Expect(suspected.find("Suspicious walls:") != std::string::npos
				&& geometry.find('o') == std::string::npos,
				"a suspected wall is marked without revealing the room behind it");
			const BreachAttempt weak = BreachRules::ResolvePhysical(
				map.grid[y][x].GetHiddenWall(direction), 1, 1);
			const BreachAttempt strong = BreachRules::ResolvePhysical(
				map.grid[y][x].GetHiddenWall(direction), 30, 20, "Claymore");
			Expect(weak.outcome != BreachOutcome::Opened
				&& strong.outcome == BreachOutcome::Opened,
				"central breaching gives high Strength real leverage on valid walls");
			Expect(DungeonTopology::OpenWall(map, x, y, direction)
				&& DungeonTopology::RenderKnowledge(map, x, y).find('o') != std::string::npos,
				"opening a generated wall reveals actual adjacent geometry");
			testedWall = true;
			break;
		}
	}
	Expect(testedWall, "seeded topology supplies a valid breakable wall fixture");

	GeneratedFloorTopology solid;
	solid.size = 2; solid.grid.assign(2, std::vector<Room>(2));
	solid.grid[0][0].exists = true;
	Expect(!DungeonTopology::OpenWall(solid, 0, 0, Direction::East)
		&& !solid.grid[0][1].exists,
		"attacking solid masonry cannot manufacture a nonexistent room");
	Expect(DungeonRules::ExplorationXP() == 0,
		"ordinary exploration and map coverage award no XP");
}

} // namespace

int main() {
	TestCombatRules();
	TestEntityAndRelicRules();
	TestEquipmentFoundations();
	TestStatusEffectFoundations();
	TestDungeonRules();
	TestEncounterRules();
	TestRoomAndDefinitions();
	TestSeededRng();
	TestLootGeneration();
	TestRestSites();
	TestEnemyRanksAndSpeciesScaling();
	TestEnemyBehaviorAndStatuses();
	TestSpellIdentities();
	TestEnemyIntentPresentation();
	TestMultiEnemyPresentationAndTargeting();
	TestCancelableCombatMenus();
	TestResolvedRoomMemory();
	TestMusicControls();
	TestLegacyProgressionAndRelicPools();
	TestPersistentBestiary();
	TestDungeonTopologyAndBreaching();
	TestReactiveDefenseRules();

	if (failures != 0) {
		std::cerr << failures << " regression test(s) failed.\n";
		return 1;
	}
	std::cout << "All regression tests passed.\n";
	return 0;
}
