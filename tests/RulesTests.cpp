#include "combat/CombatRules.h"
#include "combat/DefenseRules.h"
#include "combat/EncounterRules.h"
#include "combat/Spell.h"
#include "core/RelicRules.h"
#include "dungeon/DungeonRules.h"
#include "dungeon/Perception.h"
#include "dungeon/Room.h"
#include "entities/Enemy.h"
#include "entities/EnemyDefinitions.h"
#include "entities/Player.h"
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
	stats.atk = 10;
	stats.speed = 4;
	stats.intelligence = 3;
	stats.RecalculateDerived();
	Player player("Test Hero", stats);

	player.SetDefending(true);
	player.ReceiveDamage(5);
	Expect(player.GetHP() == player.GetMaxHP() - 2,
		"defense halves damage using integer truncation");
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

void TestDungeonRules() {
	const RoomContentWeights defaults =
		DungeonRules::CalculateRoomContentWeights(1, 1.0f, 1.0f);
	Expect(defaults.combat == 42 && defaults.chest == 15
		&& defaults.trap == 9 && defaults.rest == 11,
		"default floor-one room weights are preserved");

	const RoomContentWeights noTraps =
		DungeonRules::CalculateRoomContentWeights(1, 1.0f, 0.0f);
	Expect(noTraps.trap == 0 && noTraps.rest == 100,
		"zero trap multiplier is finite and disables traps");
	const RoomContentWeights lateNoTraps =
		DungeonRules::CalculateRoomContentWeights(12, 1.0f, 0.0f);
	Expect(lateNoTraps.rest == 2,
		"zero trap multiplier preserves the late-floor rest minimum");
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

	const auto& enemies = GetEnemyDefinitions();
	Expect(enemies.size() == 16, "all enemy definitions remain available");
	Expect(enemies.front().name == "Slime" && enemies.back().name == "Dragon",
		"enemy definition ordering is preserved");
	for (const EnemyDefinition& enemy : enemies) {
		Expect(enemy.minimumLevel > 0, enemy.name + " has a valid minimum level");
		Expect(enemy.minHp <= enemy.maxHp && enemy.minAttack <= enemy.maxAttack
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

void TestEnemyIntentPresentation() {
	Expect(EnemyIntent::DetermineClarity(EnemyKnowledge::None, 3)
		== IntentClarity::Veiled,
		"an unknown enemy remains veiled at low Intelligence");
	Expect(EnemyIntent::DetermineClarity(EnemyKnowledge::None, 4)
		== IntentClarity::Hinted,
		"Intelligence improves the read on an unknown enemy");
	Expect(EnemyIntent::DetermineClarity(EnemyKnowledge::Approximate, 0)
		== IntentClarity::Hinted,
		"prior bestiary knowledge reveals a behavioral tell");
	Expect(EnemyIntent::DetermineClarity(EnemyKnowledge::Full, 0)
		== IntentClarity::Exact,
		"full knowledge reveals exact intent");

	Stats stats;
	stats.maxHp = 20;
	stats.atk = 4;
	stats.speed = 2;
	stats.intelligence = 3;
	stats.maxMana = 9;
	Spell fireball{"Fireball", SpellElement::Fire, SpellTarget::Enemy, 3, 6, 2};
	Enemy enemy("Test Witch", stats, {fireball});

	TurnAction slash;
	slash.type = ActionType::Attack;
	slash.attackStyle = AttackStyle::Slash;
	const std::string veiled = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Veiled);
	const std::string hinted = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Hinted);
	const std::string clear = EnemyIntent::Describe(
		enemy, slash, IntentClarity::Clear);
	Expect(veiled.find("SLASH") == std::string::npos,
		"veiled intent does not expose the action label");
	Expect(hinted.find("sweeping") != std::string::npos,
		"hinted Slash intent has an interpretable physical tell");
	Expect(clear.find("SLASH") != std::string::npos,
		"clear intent names the attack style");

	TurnAction cast;
	cast.type = ActionType::CastSpell;
	cast.spellIndex = 0;
	Expect(EnemyIntent::Describe(enemy, cast, IntentClarity::Exact)
		.find("Fireball") != std::string::npos,
		"exact spell intent names the committed spell");

	Expect(CombatDisplay::MakeMeter(5, 10, 10) == "[#####-----]",
		"combat meters represent health proportionally");
	Expect(CombatDisplay::MakeMeter(-2, 10, 4) == "[----]",
		"combat meters clamp negative values safely");
}

void TestMultiEnemyPresentationAndTargeting() {
	Stats playerStats;
	playerStats.maxHp = 24;
	playerStats.atk = 5;
	playerStats.speed = 4;
	playerStats.intelligence = 3;
	playerStats.maxMana = 8;
	Player player("Test Hero", playerStats);

	Stats enemyStats;
	enemyStats.maxHp = 10;
	enemyStats.atk = 3;
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
		{false, false}, {-1, 0, 1}, {firstIntent, secondIntent}, {true, true});
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
	stats.atk = 4;
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
	stats.atk = 4;
	stats.speed = 3;
	stats.intelligence = 1;
	stats.maxMana = 3;
	Player player("Test Hero", stats);

	std::vector<std::vector<Room>> grid(2, std::vector<Room>(2));
	Room& current = grid[0][0];
	current.x = 0;
	current.y = 0;
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
	Expect(profile.GetLegacyRank() == 1
		&& profile.GetUnlockedRelics().size() == 10,
		"a new profile starts at rank one with the original relic catalogue unlocked");
	Expect(!profile.IsRelicUnlocked(RelicId::HuntersLens),
		"new progression relics begin locked");

	CompletedRun run;
	run.floorsCleared = 4;
	run.highestFloorReached = 5;
	run.enemiesDefeated = 10;
	const LegacyReward reward = profile.CompleteRun(run);
	Expect(reward.xpEarned == 210 && profile.GetLegacyRank() == 3,
		"a substantial floor-five run reaches legacy rank three");
	Expect(profile.IsRelicUnlocked(RelicId::HuntersLens)
		&& profile.IsRelicUnlocked(RelicId::RiposteSeal),
		"rank progression unlocks each intervening relic tier");
	Expect(reward.newlyUnlockedRelics.size() == 4,
		"rank jumps report every newly unlocked relic");

	const std::string saved = profile.Serialize();
	PlayerProfile loaded;
	std::string parseError;
	Expect(PlayerProfile::Deserialize(saved, loaded, &parseError),
		"a serialized legacy profile can be loaded");
	Expect(loaded.GetLegacyXP() == profile.GetLegacyXP()
		&& loaded.GetRunCount() == 1
		&& loaded.GetHighestFloor() == 5
		&& loaded.IsRelicUnlocked(RelicId::RiposteSeal),
		"legacy profile round-tripping preserves progression and unlocks");

	Expect(!RelicRules::IsAvailable(RelicId::ExecutionersEdge, 10, 3)
		&& RelicRules::IsAvailable(RelicId::ExecutionersEdge, 1, 4),
		"rare original relics enter the run pool only at their minimum depth");
	Expect(!RelicRules::IsAvailable(RelicId::AegisCoil, 2, 8)
		&& RelicRules::IsAvailable(RelicId::AegisCoil, 3, 3),
		"legacy and floor requirements both gate progression relics");
	const std::vector<RelicId> eligible = RelicRules::BuildEligiblePool(
		1, 1, {RelicId::LuckyCoin});
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
	const DefenseChallenge slowSlash = DefenseRules::BuildChallenge(
		slash, nullptr, 2, 3, {'W', 'A'});
	const DefenseChallenge fastSlash = DefenseRules::BuildChallenge(
		slash, nullptr, 8, 25, {'S', 'D'});
	Expect(slowSlash.cues.size() == 2 && slowSlash.cues[0].key == 'W',
		"slash defense patterns contain two generated cues");
	Expect(fastSlash.cues[0].fallDurationMs < slowSlash.cues[0].fallDurationMs
		&& fastSlash.blockRadiusMs < slowSlash.blockRadiusMs,
		"faster enemies produce faster patterns and narrower block windows");
}

} // namespace

int main() {
	TestCombatRules();
	TestEntityAndRelicRules();
	TestDungeonRules();
	TestEncounterRules();
	TestRoomAndDefinitions();
	TestSeededRng();
	TestEnemyIntentPresentation();
	TestMultiEnemyPresentationAndTargeting();
	TestCancelableCombatMenus();
	TestResolvedRoomMemory();
	TestMusicControls();
	TestLegacyProgressionAndRelicPools();
	TestReactiveDefenseRules();

	if (failures != 0) {
		std::cerr << failures << " regression test(s) failed.\n";
		return 1;
	}
	std::cout << "All regression tests passed.\n";
	return 0;
}
