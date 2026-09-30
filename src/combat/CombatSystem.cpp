// CombatSystem.cpp
// -----------------
// Resolves turn-based combat between the player and an enemy.
//
// Combat flow: actors resolve in initiative order each round.
// Player defense is reactive: commit to Defend, then catch falling key cues.
// Each defended beat reduces its share of the hit; an all-perfect sequence parries.
//
//
// Attack style balance:
//   Slash:  1.0x STR, 15% crit (1.5x). Reliable.
//   Thrust: 0.8x normally, 1.0x vs defenders. Ignores defense.
//   Bash:   1.3x STR, 15% whiff + self-damage. High risk/reward.
//
// Reactive Defense:
//   PERFECT beat: contributes no damage.
//   BLOCK beat:   contributes half damage.
//   MISSED beat:  contributes full damage for that beat only.
//   ALL PERFECT:  automatic counter.
// Enemy guards retain hidden directional stances.
//
// Enemy intent is planned at round start. Species behavior determines whether
// the tell is already committed; uncommitted plans may still change.

#include "CombatSystem.h"
#include "CombatRules.h"
#include "DefenseRules.h"
#include "SpellRules.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "core/GameStats.h"
#include "core/Bestiary.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include "core/Relic.h"
#include "presentation/CombatDisplay.h"
#include "presentation/CombatMenu.h"
#include "presentation/DefenseQTE.h"
#include "equipment/Equipment.h"
#include <iostream>
#include <algorithm>
#include <set>

// ---- Flavor text helpers ----

static std::string PickRandom(const std::vector<std::string>& lines) {
	static RNG rng;
	return lines[rng.NextInt(0, static_cast<int>(lines.size()) - 1)];
}

// ---- On-hit relic effects ----
// attacker dealt `dmgDealt` to target. Handles:
//   Vampiric Fang    (player attacking): heal 20% of damage dealt
//   Thorned Carapace (player defending): attacker takes 3 damage
static void ApplyOnHitRelics(Entity& attacker, Entity& target, int dmgDealt, bool attackerIsPlayer) {
	if (dmgDealt <= 0) return;

	if (attackerIsPlayer) {
		Player& p = static_cast<Player&>(attacker);
		if (p.HasRelic(RelicId::VampiricFang)) {
			int heal = std::max(1, dmgDealt / 5);
			p.Heal(heal);
			Console::PrintSlow("  (Vampiric Fang drinks deep: +" + std::to_string(heal) + " HP)");
		}
	}
	else {
		// Enemy hit the player
		Player& p = static_cast<Player&>(target);
		if (p.HasRelic(RelicId::ThornedCarapace) && attacker.IsAlive()) {
			attacker.ReceiveDamage(3);
			Console::PrintSlow("  (Thorned Carapace bites back: 3 damage to "
				+ attacker.GetName() + "!)");
		}
	}
}

static bool ApplyWindForm(const Player& player, RNG& rng);
static void ApplyWeaponEnchantments(Player& player, Entity& target,
	int damageDealt, RNG& rng);
static void ApplyEnemyOnHitStatus(Entity& attacker, Player& target,
	int damageDealt, Bestiary* bestiary);
static void ApplySpellOnHit(Entity& caster, Entity& target, const Spell& spell,
	int damageDealt, bool casterIsPlayer, RNG& rng, GameStats& stats,
	Bestiary* bestiary);

static const char* DefensePatternDiscovery(EnemyArchetype archetype) {
	switch (archetype) {
	case EnemyArchetype::Witch: return "serpentine cadence witnessed";
	case EnemyArchetype::DarkMage: return "sparse grouped cadence witnessed";
	case EnemyArchetype::Giant: return "clustered attack witnessed";
	case EnemyArchetype::Werewolf: return "burst rhythm witnessed";
	case EnemyArchetype::Dragon: return "mixed apex pattern witnessed";
	default: return "defense cadence witnessed";
	}
}

struct CombatRuntime {
	bool aegisCoilUsedThisRound = false;
	bool defenseQteOccurred = false;
};

static bool ResolveReactiveDefense(Entity& attacker, Entity& target,
	const TurnAction& action, const Spell* spell, int damage,
	GameStats& stats, CombatRuntime* runtime, Bestiary* bestiary) {
	if (!target.IsDefending()) return false;
	Player* player = dynamic_cast<Player*>(&target);
	if (!player) return false;

	const Enemy* enemy = dynamic_cast<const Enemy*>(&attacker);
	static RNG patternRng;
	const DefenseChallenge challenge = DefenseRules::BuildChallenge(
		action, spell,
		enemy ? enemy->GetArchetype() : EnemyArchetype::Slime,
		enemy ? enemy->GetRank() : 1,
		attacker.GetSpeed(), attacker.GetStrength(), player->GetSpeed(),
		patternRng.NextInt(0, 3));
	if (enemy && bestiary) bestiary->RecordBehaviorObserved(enemy->GetName(),
		DefensePatternDiscovery(enemy->GetArchetype()));
	const DefenseOutcome defense = DefenseQTE::Run(challenge);
	const DefenseResult result = defense.result;
	if (runtime) runtime->defenseQteOccurred = true;

	if (result == DefenseResult::PerfectParry) {
		Console::PrintSlow("  ** PERFECT PARRY! No damage received. **");
		const int counter = action.type == ActionType::CastSpell
			? CombatRules::MagicCounterDamage(player->GetStrength())
			: CombatRules::PhysicalCounterDamage(player->GetStrength());
		attacker.ReceiveDamage(counter);
		stats.totalDamageDealt += counter;
		Console::PrintSlow("  " + player->GetName() + " counters "
			+ attacker.GetName() + " for " + std::to_string(counter) + " damage!");
		if (player->HasRelic(RelicId::RiposteSeal)) {
			player->ApplyPowerBuff(20, 1);
			Console::PrintSlow("  (Riposte Seal: your next damaging action is empowered.)");
		}
		return true;
	}

	const int hpBefore = player->GetHP();
	if (result == DefenseResult::Block) {
		const bool wasDefending = player->IsDefending();
		player->SetDefending(false);
		player->ReceiveDamage(DefenseRules::DamageAfterDefense(damage, defense));
		player->SetDefending(wasDefending);
		const int damageTaken = hpBefore - player->GetHP();
		stats.totalDamageTaken += damageTaken;
		const std::string guardLabel = defense.missCount > 0 ? "PARTIAL GUARD" : "BLOCK";
		Console::PrintSlow("  ** " + guardLabel + "! Damage reduced to "
			+ std::to_string(damageTaken) + ". **");
		ApplyOnHitRelics(attacker, *player, damageTaken, false);
		if (spell) {
			static RNG spellRng;
			ApplySpellOnHit(attacker, *player, *spell, damageTaken, false, spellRng,
				stats, bestiary);
		}
		else ApplyEnemyOnHitStatus(attacker, *player, damageTaken, bestiary);
		if (runtime && !runtime->aegisCoilUsedThisRound
			&& player->HasRelic(RelicId::AegisCoil)) {
			runtime->aegisCoilUsedThisRound = true;
			const int manaBefore = player->GetMana();
			player->RestoreMana(1);
			if (player->GetMana() > manaBefore) {
				Console::PrintSlow("  (Aegis Coil resonates: +1 Mana.)");
			}
		}
		return true;
	}

	const bool wasDefending = player->IsDefending();
	player->SetDefending(false);
	player->ReceiveDamage(DefenseRules::DamageAfterDefense(damage, defense));
	player->SetDefending(wasDefending);
	const int damageTaken = hpBefore - player->GetHP();
	stats.totalDamageTaken += damageTaken;
	Console::PrintSlow("  ** GUARD BREAK! Full damage: "
		+ std::to_string(damageTaken) + ". **");
	ApplyOnHitRelics(attacker, *player, damageTaken, false);
	if (spell) {
		static RNG spellRng;
		ApplySpellOnHit(attacker, *player, *spell, damageTaken, false, spellRng,
			stats, bestiary);
	}
	else ApplyEnemyOnHitStatus(attacker, *player, damageTaken, bestiary);
	return true;
}

struct SpellHitResult {
	int damage = 0;
	bool hitWeakness = false;
};

static SpellHitResult ResolveDamageSpellHit(Entity& actor, Entity& target,
	const TurnAction& action, const Spell& spell, int damage, GameStats& stats,
	Enemy* enemyTarget, bool isPlayer, Bestiary* bestiary,
	CombatRuntime* runtime, RNG& rng) {
	SpellHitResult result;
	result.hitWeakness = enemyTarget && spell.element == enemyTarget->GetWeakness();
	if (result.hitWeakness) {
		damage = static_cast<int>(damage * CombatRules::WeaknessMultiplier);
		if (isPlayer && bestiary) {
			const bool discovered = bestiary->RecordWeaknessDiscovered(enemyTarget->GetName());
			if (discovered) Console::PrintSlow("  ** Weakness discovered: "
				+ enemyTarget->GetName() + " is weak to " + spell.GetElementName()
				+ "! Added to bestiary! **");
		}
	}
	Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
		+ " [" + spell.GetElementName() + "] at " + target.GetName() + " for "
		+ std::to_string(damage) + " damage!"
		+ (result.hitWeakness ? " It's super effective!" : ""));

	if (!isPlayer && ResolveReactiveDefense(actor, target, action, &spell,
		damage, stats, runtime, bestiary)) return result;

	if (target.IsDefending() && target.GetDefenseStance() == DefenseStance::AntiMagic) {
		Console::PrintSlow("  ** MAGIC PARRY! " + target.GetName()
			+ " deflects the spell! **");
		const int counter = CombatRules::MagicCounterDamage(target.GetStrength());
		actor.ReceiveDamage(counter);
		if (isPlayer) stats.totalDamageTaken += counter;
		else stats.totalDamageDealt += counter;
		Console::PrintSlow("  " + target.GetName() + " retaliates for "
			+ std::to_string(counter) + " damage!");
		return result;
	}

	int appliedDamage = damage;
	if (target.IsDefending()) {
		appliedDamage = SpellRules::DamageAgainstConventionalGuard(spell, damage);
		const bool wasDefending = target.IsDefending();
		target.SetDefending(false);
		const int before = target.GetHP();
		target.ReceiveDamage(appliedDamage);
		target.SetDefending(wasDefending);
		result.damage = before - target.GetHP();
		Console::PrintSlow(appliedDamage > damage / 2
			? "  (The spell partially pierces the guard!)"
			: "  (Halved by defense!)");
	}
	else {
		const int before = target.GetHP();
		target.ReceiveDamage(appliedDamage);
		result.damage = before - target.GetHP();
	}

	if (isPlayer) stats.totalDamageDealt += result.damage;
	else {
		stats.totalDamageTaken += result.damage;
		ApplyOnHitRelics(actor, target, result.damage, false);
	}
	ApplySpellOnHit(actor, target, spell, result.damage, isPlayer, rng, stats, bestiary);
	return result;
}

static void RecordEnemyDefeat(Player& player, const Enemy& enemy,
	GameStats& stats, Bestiary& bestiary) {
	++stats.totalKills;
	bestiary.RecordKill(enemy.GetName());
	if (player.HasRelic(RelicId::BloodLedger)) {
		const int hpBefore = player.GetHP();
		player.Heal(3);
		const int restored = player.GetHP() - hpBefore;
		if (restored > 0) {
			Console::PrintSlow("  (Blood Ledger closes a name: +"
				+ std::to_string(restored) + " HP.)");
		}
	}
}

// Resolve one already-selected action. Optional encounter context is passed in
// only for effects that need to update persistent knowledge or other enemies.
static void ExecuteAction(Entity& actor, Entity& target, const TurnAction& action,
	GameStats& stats, Enemy* enemyTarget, bool isPlayer, Bestiary* bestiary = nullptr,
	CombatRuntime* runtime = nullptr, std::vector<Enemy>* encounter = nullptr) {

	switch (action.type) {

	case ActionType::Attack: {
		static RNG rng;
		const int baseStrength = actor.GetStrength();
		const int baseDamage = isPlayer
			? static_cast<Player&>(actor).GetWeaponDamage()
			: baseStrength;
		int dmg = baseDamage;
		bool missed = false;
		bool crit = false;

		// -- Calculate damage based on style --
		switch (action.attackStyle) {
		case AttackStyle::Slash: {
			dmg = baseDamage;
			float critChance = CombatRules::SlashCritChance;
			// Lucky Coin: doubled crit chance for the player
			if (isPlayer && static_cast<Player&>(actor).HasRelic(RelicId::LuckyCoin))
				critChance = CombatRules::LuckyCoinCritChance;
			if (rng.Chance(critChance)) {
				dmg = static_cast<int>(baseDamage * CombatRules::SlashCritMultiplier);
				crit = true;
			}
			break;
		}
		case AttackStyle::Thrust:
			dmg = target.IsDefending()
				? baseDamage                         // Full damage vs defenders
				: static_cast<int>(baseDamage * CombatRules::ThrustDamageMultiplier);
			if (dmg < 1) dmg = 1;
			break;
		case AttackStyle::Bash:
			if (rng.Chance(CombatRules::BashMissChance)) {
				missed = true;
			} else {
				dmg = static_cast<int>(baseDamage * CombatRules::BashDamageMultiplier);
			}
			break;
		}

		if (isPlayer) stats.RecordPhysicalAttack();

		// Executioner's Edge: +50% physical damage vs enemies below 30% HP
		if (isPlayer && !missed && enemyTarget
			&& static_cast<Player&>(actor).HasRelic(RelicId::ExecutionersEdge)
			&& enemyTarget->GetHP() * 10 < enemyTarget->GetMaxHP() * 3) {
			dmg = static_cast<int>(dmg * CombatRules::ExecutionerMultiplier);
			Console::PrintSlow("  (Executioner's Edge: the wounded foe is exposed!)");
		}

		// -- Bash whiff --
		if (missed) {
			int selfDmg = std::max(1, static_cast<int>(baseStrength * CombatRules::BashRecoilMultiplier));
			Console::PrintSlow("  " + PickRandom({
				actor.GetName() + " swings wildly and loses balance!",
				actor.GetName() + " overcommits and stumbles!",
				actor.GetName() + " puts too much force behind the blow and misses!",
				actor.GetName() + "'s heavy strike goes wide!",
			}));
			Console::PrintSlow("  " + PickRandom({
				"The momentum hurts! " + std::to_string(selfDmg) + " self-damage!",
				"The recoil deals " + std::to_string(selfDmg) + " damage to " + actor.GetName() + "!",
				actor.GetName() + " takes " + std::to_string(selfDmg) + " damage from the failed swing!",
			}));
			actor.ReceiveDamage(selfDmg);
			if (isPlayer) stats.totalDamageTaken += selfDmg;
			if (!isPlayer) stats.totalDamageDealt += selfDmg;
			break;
		}

		if (isPlayer && ApplyWindForm(static_cast<Player&>(actor), rng)) {
			dmg *= 2;
			Console::PrintSlow("  (Wind-form catches the strike: double damage!)");
		}

		const int unempoweredDamage = dmg;
		dmg = actor.ConsumePowerBuff(dmg);
		if (dmg > unempoweredDamage) {
			Console::PrintSlow("  (Empower surges: "
				+ std::to_string(dmg - unempoweredDamage) + " bonus damage!)");
		}

		// -- Hit message (varied per style) --
		if (!isPlayer) {
			Player* playerTarget = dynamic_cast<Player*>(&target);
			if (playerTarget) {
				dmg = EquipmentRules::MitigatePhysicalDamage(dmg, playerTarget->GetArmor());
			}
		}

		std::string hitMsg;
		switch (action.attackStyle) {
		case AttackStyle::Slash:
			hitMsg = PickRandom({
				actor.GetName() + " slashes for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " cuts across for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " delivers a sweeping slash for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " carves into the foe for " + std::to_string(dmg) + " damage!",
			});
			break;
		case AttackStyle::Thrust:
			hitMsg = PickRandom({
				actor.GetName() + " thrusts precisely for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " drives a precise stab for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " lunges with pinpoint accuracy for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " pierces forward for " + std::to_string(dmg) + " damage!",
			});
			break;
		case AttackStyle::Bash:
			hitMsg = PickRandom({
				actor.GetName() + " lands a crushing blow for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " smashes with tremendous force for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " brings down a devastating strike for " + std::to_string(dmg) + " damage!",
				actor.GetName() + " hammers the target for " + std::to_string(dmg) + " damage!",
			});
			break;
		}
		if (crit) {
			hitMsg += PickRandom({
				" CRITICAL HIT!",
				" A devastating blow!",
				" Right on target!",
				" A vicious strike!",
			});
		}
		Console::PrintSlow("  " + hitMsg);

		// -- Apply damage, handle defense/parry --
		// A correct stance parries ANY physical style, including Thrust.
		// Thrust's edge: against a WRONG stance it pierces (full damage,
		// no halving), where Slash/Bash get halved.
		bool ignoreDefense = (action.attackStyle == AttackStyle::Thrust);
		if (!isPlayer && ResolveReactiveDefense(actor, target, action, nullptr,
			dmg, stats, runtime, bestiary)) {
			break;
		}

		if (target.IsDefending()) {
			if (CombatRules::IsPhysicalParry(target.GetDefenseStance(), action.attackStyle)) {
				// PARRY! Zero damage, full counter
				Console::PrintSlow("  " + PickRandom({
					"** PARRY! " + target.GetName() + " read the attack perfectly! **",
					"** PARRY! " + target.GetName() + " saw it coming and deflects! **",
					"** PERFECT BLOCK! " + target.GetName() + " turns the attack aside! **",
					"** PARRY! " + target.GetName() + " catches the blow and turns it! **",
				}));
				int counter = CombatRules::PhysicalCounterDamage(target.GetStrength());
				if (isPlayer) {
					counter = EquipmentRules::MitigatePhysicalDamage(
						counter, static_cast<Player&>(actor).GetArmor());
				}
				Console::PrintSlow("  " + PickRandom({
					target.GetName() + " strikes back for " + std::to_string(counter) + " damage!",
					target.GetName() + " retaliates with a devastating " + std::to_string(counter) + " damage counter!",
					target.GetName() + " punishes the opening for " + std::to_string(counter) + " damage!",
				}));
				actor.ReceiveDamage(counter);
				if (isPlayer) stats.totalDamageTaken += counter;
				if (!isPlayer) stats.totalDamageDealt += counter;
				// Defender takes 0 damage
			}
			else if (ignoreDefense) {
				// Thrust vs a wrong stance: pierces entirely
				bool wasDefending = target.IsDefending();
				target.SetDefending(false);
				target.ReceiveDamage(dmg);
				target.SetDefending(wasDefending);
				Console::PrintSlow("  (Thrust pierces through the defense!)");
				if (isPlayer) stats.totalDamageDealt += dmg;
				if (!isPlayer) stats.totalDamageTaken += dmg;
				ApplyOnHitRelics(actor, target, dmg, isPlayer);
				if (isPlayer) ApplyWeaponEnchantments(
					static_cast<Player&>(actor), target, dmg, rng);
				else ApplyEnemyOnHitStatus(actor, static_cast<Player&>(target), dmg, bestiary);
			}
			else {
				// Wrong guess vs Slash/Bash: halve damage, no counter
				int actualDmg = dmg / 2;
				target.ReceiveDamage(dmg); // ReceiveDamage halves internally
				Console::PrintSlow("  (Halved by defense!)");
				if (isPlayer) stats.totalDamageDealt += actualDmg;
				if (!isPlayer) stats.totalDamageTaken += actualDmg;
				ApplyOnHitRelics(actor, target, actualDmg, isPlayer);
				if (isPlayer) ApplyWeaponEnchantments(
					static_cast<Player&>(actor), target, actualDmg, rng);
				else ApplyEnemyOnHitStatus(actor, static_cast<Player&>(target), actualDmg, bestiary);
			}
		}
		else {
			// Not defending: full damage
			target.ReceiveDamage(dmg);
			if (isPlayer) stats.totalDamageDealt += dmg;
			if (!isPlayer) stats.totalDamageTaken += dmg;
			ApplyOnHitRelics(actor, target, dmg, isPlayer);
			if (isPlayer) ApplyWeaponEnchantments(
				static_cast<Player&>(actor), target, dmg, rng);
			else ApplyEnemyOnHitStatus(actor, static_cast<Player&>(target), dmg, bestiary);
		}
		break;
	}

	case ActionType::Defend: {
		actor.SetDefending(true);
		if (isPlayer) {
			Console::PrintSlow("  " + actor.GetName()
				+ " enters a reactive guard. Read the lane and catch the impact!");
		}
		else {
			actor.SetDefenseStance(action.defenseStance);
			// Enemy stance is hidden
			Console::PrintSlow("  " + PickRandom({
				actor.GetName() + " settles into a guarded position.",
				actor.GetName() + " readies a defensive stance.",
				actor.GetName() + " watches your movements carefully.",
				actor.GetName() + " braces for what's coming.",
				actor.GetName() + " tightens their guard.",
			}));
		}
		break;
	}

	case ActionType::CastSpell: {
		const auto& spells = actor.GetKnownSpells();
		if (action.spellIndex < 0 || action.spellIndex >= static_cast<int>(spells.size())) {
			Console::PrintSlow("  Spell fizzles...");
			break;
		}
		const Spell& spell = spells[action.spellIndex];
		if (!isPlayer && bestiary)
			bestiary->RecordSpellObserved(actor.GetName(), spell.name);
		int manaCost = isPlayer
			? static_cast<Player&>(actor).GetEffectiveManaCost(spell)
			: spell.manaCost;
		actor.UseMana(manaCost);

		if (isPlayer) stats.RecordSpellCast(spell.name);

		if (spell.effect == SpellEffect::Damage) {
			const int casterRank = isPlayer
				? static_cast<Player&>(actor).GetLevel()
				: static_cast<Enemy&>(actor).GetRank();
			const int spellPowerBonus = isPlayer
				? static_cast<Player&>(actor).GetSpellPowerBonus() : 0;
			const int powerPercent = actor.GetPowerBuff().remainingHits > 0
				? actor.GetPowerBuff().percentBonus : 0;
			if (powerPercent > 0) {
				actor.ConsumePowerBuff(100);
				Console::PrintSlow("  (Empower surges through the spell.)");
			}

			auto calculateDamage = [&](int jumpIndex) {
				int damage = SpellRules::CalculateDamage(spell, actor.GetStrength(),
					actor.GetSpeed(), actor.GetIntelligence(), casterRank, jumpIndex);
				damage += spellPowerBonus;
				return damage * (100 + powerPercent) / 100;
			};

			bool prismTriggered = false;
			static RNG spellRng;
			if (isPlayer && encounter
				&& (SpellRules::IsMultiTarget(spell) || SpellRules::IsChain(spell))) {
				std::vector<Enemy*> targets;
				if (SpellRules::IsChain(spell) && enemyTarget && enemyTarget->IsAlive()) {
					targets.push_back(enemyTarget);
				}
				for (Enemy& enemy : *encounter) {
					if (!enemy.IsAlive()
						|| (!targets.empty() && &enemy == targets.front())) continue;
					targets.push_back(&enemy);
				}
				for (std::size_t i = 0; i < targets.size() && actor.IsAlive(); ++i) {
					const int jump = SpellRules::IsChain(spell) ? static_cast<int>(i) : 0;
					const SpellHitResult hit = ResolveDamageSpellHit(actor, *targets[i], action,
						spell, calculateDamage(jump), stats, targets[i], true,
						bestiary, runtime, spellRng);
					prismTriggered = prismTriggered || (hit.hitWeakness && hit.damage > 0);
				}
			}
			else {
				const SpellHitResult hit = ResolveDamageSpellHit(actor, target, action,
					spell, calculateDamage(0), stats, enemyTarget, isPlayer,
					bestiary, runtime, spellRng);
				prismTriggered = hit.hitWeakness && hit.damage > 0;
			}
			if (isPlayer && prismTriggered
				&& static_cast<Player&>(actor).HasRelic(RelicId::ManaPrism)) {
				Player& player = static_cast<Player&>(actor);
				const int manaBefore = player.GetMana();
				player.RestoreMana(1);
				if (player.GetMana() > manaBefore) {
					Console::PrintSlow("  (Mana Prism refracts the weakness: +1 Mana.)");
				}
			}
		}
		else if (spell.effect == SpellEffect::Heal) {
			int heal = SpellRules::CalculateHealing(
				spell, actor.GetMaxHP(), actor.GetIntelligence());
			if (isPlayer) heal += static_cast<Player&>(actor).GetSpellPowerBonus();
			const int hpBefore = actor.GetHP();
			actor.Heal(heal);
			const int restored = actor.GetHP() - hpBefore;
			if (isPlayer) stats.totalHealing += restored;
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and restores " + std::to_string(restored) + " HP!");
		}
		else if (spell.effect == SpellEffect::Regeneration) {
			int stacks = SpellRules::RegenerationStacks(
				spell, actor.GetMaxHP(), actor.GetIntelligence());
			if (isPlayer) stacks += static_cast<Player&>(actor).GetSpellPowerBonus() / 4;
			actor.GetStatuses().ApplyRegeneration(stacks, spell.duration);
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and gains Regeneration " + std::to_string(stacks) + "!");
		}
		else if (spell.effect == SpellEffect::Empower) {
			actor.ApplyPowerBuff(spell.power, spell.duration);
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and becomes empowered: +" + std::to_string(spell.power)
				+ "% damage for the next " + std::to_string(spell.duration)
				+ " damaging actions!");
		}
		break;
	}

	case ActionType::UseItem:
		break;
	case ActionType::Inspect:
		break;
	case ActionType::None:
		Console::PrintSlow("  " + actor.GetName() + " does nothing.");
		if (!isPlayer && bestiary)
			bestiary->RecordBehaviorObserved(actor.GetName(), "recovery pause witnessed");
		break;
	}
}

// ---- Inspect enemy (hidden d20+INT roll) ----

static EnemyKnowledge InspectEnemy(const Player& player, Enemy& enemy,
	EnemyKnowledge currentKnowledge) {
	static RNG rng;
	int base = rng.NextInt(1, 20);
	int total = base + player.GetIntelligence();

	if (base == 1) {
		Console::PrintSlow("  " + PickRandom({
			"You try to study the " + enemy.GetName() + "... but you can't focus at all.",
			"You squint at the " + enemy.GetName() + " but your mind goes blank.",
			"You attempt to read the " + enemy.GetName() + "... nothing. Absolutely nothing.",
		}));
		return currentKnowledge;
	}
	else if (total < 10) {
		Console::PrintSlow("  " + PickRandom({
			"You squint at the " + enemy.GetName() + "... you can't make out much.",
			"You get a vague sense of the " + enemy.GetName() + ", but details elude you.",
			"The " + enemy.GetName() + " is hard to read. You pick up only fragments.",
		}));
		if (currentKnowledge < EnemyKnowledge::Approximate)
			return EnemyKnowledge::Approximate;
		return currentKnowledge;
	}
	else if (total < 18) {
		Console::PrintSlow("  " + PickRandom({
			"You study the " + enemy.GetName() + " carefully and get a read on it.",
			"Details emerge as you focus on the " + enemy.GetName() + ".",
			"You pick apart the " + enemy.GetName() + "'s stance and movements.",
		}));
		if (currentKnowledge < EnemyKnowledge::Partial)
			return EnemyKnowledge::Partial;
		return currentKnowledge;
	}
	else {
		Console::PrintSlow("  " + PickRandom({
			"You lock eyes with the " + enemy.GetName() + " and see through it completely.",
			"Every detail of the " + enemy.GetName() + " becomes crystal clear.",
			"The " + enemy.GetName() + " has no secrets from you now.",
		}));
		return EnemyKnowledge::Full;
	}
}

// ---- Death-save Heartbeat QTE ----

static bool AttemptDeathSave(Player& player, bool& deathSaveUsed) {
	if (player.IsAlive() || deathSaveUsed) return false;

	deathSaveUsed = true;

	Console::WaitForEnter();
	Console::Clear();

	int saveCount = player.GetDeathSaveCount();

	// Generate the heartbeat sequence
	static RNG qteRng;
	int seqLen = std::min(12, 4 + saveCount * 2);
	int windowMs = std::max(400, 1000 - saveCount * 100);
	int beatMs = 600;

	// Phoenix Feather: the heart beats slower at death's door
	if (player.HasRelic(RelicId::PhoenixFeather)) {
		windowMs += 250;
		Console::PrintSlow("  (The Phoenix Feather glows warm against your chest...)", 400);
	}

	std::vector<int> sequence;
	for (int i = 0; i < seqLen; ++i) {
		sequence.push_back(qteRng.NextInt(1, 3));
	}

	// Dramatic buildup
	Console::PrintSlow("", 200);
	Console::PrintSlow("  ...", 800);
	Console::PrintSlow("", 400);
	Console::PrintSlow("  " + PickRandom({
		"On their last breath, " + player.GetName() + " clings to life desperately,",
		"Falling to one knee, " + player.GetName() + " refuses to give in,",
		"The world goes dark... but " + player.GetName() + "'s heart still beats,",
		"Time slows. " + player.GetName() + " feels every heartbeat like thunder,",
	}), 1000);
	Console::PrintSlow("  counting every beat of their heart to survive.", 1200);
	Console::PrintSlow("", 600);
	Console::PrintSlow("  Match each number! (Press 1, 2, or 3)", 400);
	if (saveCount > 0) {
		Console::PrintSlow("  (Death save #" + std::to_string(saveCount + 1)
			+ " — the window is tighter...)", 400);
	}
	Console::PrintSlow("", 300);

	bool survived = Console::HeartbeatQTE(sequence, beatMs, windowMs);

	if (survived) {
		int reviveHp = player.HasRelic(RelicId::LastEmber)
			? std::max(1, player.GetMaxHP() / 5)
			: std::max(1, player.GetMaxHP() / 15);
		player.Heal(reviveHp);
		player.IncrementDeathSave();
		if (player.HasRelic(RelicId::LastEmber)) {
			Console::PrintSlow("  (Last Ember flares and pulls you farther from death.)");
		}

		Console::PrintSlow("", 300);
		Console::PrintSlow("  " + PickRandom({
			"** " + player.GetName() + " REFUSES DEATH! **",
			"** Not today! " + player.GetName() + " rises! **",
			"** Sheer willpower! " + player.GetName() + " will NOT fall here! **",
			"** Through gritted teeth, " + player.GetName() + " stands again! **",
			"** The heart beats on! " + player.GetName() + " lives! **",
		}));
		Console::PrintSlow("  Recovered " + std::to_string(reviveHp) + " HP!");
		Console::PrintSlow("");

		Console::WaitForEnter();
		Console::Clear();
		return true;
	}
	else {
		Console::PrintSlow("", 300);
		Console::PrintSlow("  " + PickRandom({
			"...the heartbeat fades. Silence.",
			"...the rhythm breaks. " + player.GetName() + " falls.",
			"...one final beat. Then nothing.",
			"...the light leaves " + player.GetName() + "'s eyes.",
		}));
		Console::PrintSlow("");
		return false;
	}
}

// ---- Main combat loop ----

static int CountLivingEnemies(const std::vector<Enemy>& enemies) {
	return static_cast<int>(std::count_if(enemies.begin(), enemies.end(),
		[](const Enemy& enemy) { return enemy.IsAlive(); }));
}

static int FirstLivingEnemy(const std::vector<Enemy>& enemies) {
	for (size_t i = 0; i < enemies.size(); ++i) {
		if (enemies[i].IsAlive()) return static_cast<int>(i);
	}
	return -1;
}

static bool ActionNeedsEnemyTarget(const Player& player, const TurnAction& action) {
	if (action.type == ActionType::Attack || action.type == ActionType::Inspect) return true;
	if (action.type != ActionType::CastSpell) return false;
	const auto& spells = player.GetKnownSpells();
	return action.spellIndex >= 0
		&& action.spellIndex < static_cast<int>(spells.size())
		&& spells[action.spellIndex].effect == SpellEffect::Damage
		&& spells[action.spellIndex].target != SpellTarget::AllEnemies;
}

static std::vector<int> BuildInitiativeOrder(const Player& player,
	const std::vector<Enemy>& enemies) {
	std::vector<int> order = {-1};
	for (size_t i = 0; i < enemies.size(); ++i) {
		if (enemies[i].IsAlive()) order.push_back(static_cast<int>(i));
	}
	std::stable_sort(order.begin(), order.end(), [&](int left, int right) {
		const int leftSpeed = left < 0 ? player.GetSpeed() : enemies[left].GetSpeed();
		const int rightSpeed = right < 0 ? player.GetSpeed() : enemies[right].GetSpeed();
		if (leftSpeed != rightSpeed) return leftSpeed > rightSpeed;
		if (left < 0 || right < 0) return left < 0;
		return left < right;
	});
	return order;
}

static void AnnounceEnemyDefeat(const std::vector<Enemy>& enemies, int enemyIndex) {
	const int remaining = CountLivingEnemies(enemies);
	Console::PrintSlow("  ** [E" + std::to_string(enemyIndex + 1) + "] "
		+ enemies[enemyIndex].GetName() + " is defeated! **");
	if (remaining > 0) {
		Console::PrintSlow("  " + std::to_string(remaining)
			+ (remaining == 1 ? " enemy remains." : " enemies remain."));
	}
}

static bool ApplyWindForm(const Player& player, RNG& rng) {
	const auto& weapon = player.GetEquipment().GetWeapon();
	if (!weapon) return false;
	for (const Enchantment& enchantment : weapon->GetEnchantments()) {
		if (enchantment.type == EnchantmentType::WindForm
			&& rng.Chance(static_cast<float>(EnchantmentRules::ProcChance(
				enchantment.type, enchantment.potencyRank)))) return true;
	}
	return false;
}

static void ApplyWeaponEnchantments(Player& player, Entity& target,
	int damageDealt, RNG& rng) {
	if (damageDealt <= 0) return;
	const auto& weapon = player.GetEquipment().GetWeapon();
	if (!weapon) return;
	for (const Enchantment& enchantment : weapon->GetEnchantments()) {
		if (enchantment.type == EnchantmentType::WindForm) continue;
		if (!rng.Chance(static_cast<float>(EnchantmentRules::ProcChance(
			enchantment.type, enchantment.potencyRank)))) continue;
		switch (enchantment.type) {
		case EnchantmentType::FireForm: {
			const int stacks = EnchantmentRules::BurnStacks(enchantment.potencyRank);
			target.GetStatuses().ApplyBurn(stacks);
			if (Enemy* enemy = dynamic_cast<Enemy*>(&target)) enemy->SuppressRegeneration();
			Console::PrintSlow("  (Fire-form ignites the target: Burn "
				+ std::to_string(stacks) + ".)");
			break;
		}

		case EnchantmentType::Drain: {
			const int before = player.GetHP();
			player.Heal(EnchantmentRules::DrainHealing(damageDealt));
			const int restored = player.GetHP() - before;
			if (restored > 0) Console::PrintSlow("  (Drain restores "
				+ std::to_string(restored) + " HP.)");
			break;
		}
		case EnchantmentType::IceForm:
			target.GetStatuses().ApplyFreeze();
			Console::PrintSlow("  (Ice-form freezes the target's next action.)");
			break;
		case EnchantmentType::SnakeTongue: {
			const int poison = EnchantmentRules::PoisonPotency(enchantment.potencyRank);
			target.GetStatuses().ApplyPoison(poison);
			Console::PrintSlow("  (Snake-Tongue applies Poison "
				+ std::to_string(poison) + ".)");
			break;
		}
		case EnchantmentType::WindForm: break;
		}
	}
}

static void ApplyEnemyOnHitStatus(Entity& attacker, Player& target,
	int damageDealt, Bestiary* bestiary) {
	if (damageDealt <= 0) return;
	Enemy* enemy = dynamic_cast<Enemy*>(&attacker);
	if (!enemy) return;
	static RNG rng;
	const std::optional<StatusEffect> status = enemy->RollOnHitStatus(rng);
	if (!status) return;
	if (bestiary) bestiary->RecordStatusObserved(enemy->GetName(), status->type);
	if (status->type == StatusType::Poison) {
		target.GetStatuses().ApplyPoison(status->potency);
		Console::PrintSlow("  (" + enemy->GetName() + " inflicts Poison "
			+ std::to_string(status->potency) + ".)");
	}
	else if (status->type == StatusType::Burn) {
		target.GetStatuses().ApplyBurn(status->potency);
		Console::PrintSlow("  (" + enemy->GetName() + " inflicts Burn "
			+ std::to_string(status->potency) + ".)");
	}
	else if (status->type == StatusType::Bleed) {
		target.GetStatuses().ApplyBleed(status->potency);
		const int charge = target.GetStatuses().Get(StatusType::Bleed).potency;
		Console::PrintSlow("  (" + enemy->GetName() + " builds Bleed to "
			+ std::to_string(charge) + "/"
			+ std::to_string(StatusTuning::BleedBurstThreshold) + ".)");
	}
}

static void ApplySpellOnHit(Entity& caster, Entity& target, const Spell& spell,
	int damageDealt, bool casterIsPlayer, RNG& rng, GameStats& stats,
	Bestiary* bestiary) {
	if (damageDealt <= 0) return;
	const int casterRank = casterIsPlayer
		? static_cast<Player&>(caster).GetLevel()
		: static_cast<Enemy&>(caster).GetRank();
	if (SpellRules::SuppressesRegeneration(spell)) {
		if (Enemy* enemy = dynamic_cast<Enemy*>(&target)) enemy->SuppressRegeneration();
	}
	const std::optional<SpellStatusRule> status = SpellRules::StatusRule(spell, casterRank);
	if (status && rng.Chance(static_cast<float>(status->chance))) {
		if (status->type == StatusType::Burn) {
			target.GetStatuses().ApplyBurn(status->potency, status->duration);
			Console::PrintSlow("  (" + spell.name + " applies Burn "
				+ std::to_string(status->potency) + ".)");
		}
		else if (status->type == StatusType::Freeze) {
			target.GetStatuses().ApplyFreeze(status->potency);
			Console::PrintSlow("  (" + spell.name + " disrupts the target's next action.)");
		}
		else if (status->type == StatusType::Poison) {
			target.GetStatuses().ApplyPoison(status->potency);
			Console::PrintSlow("  (" + spell.name + " applies Poison "
				+ std::to_string(status->potency) + ".)");
		}
		if (!casterIsPlayer && bestiary) {
			bestiary->RecordStatusObserved(caster.GetName(), status->type);
		}
	}
	const int drained = SpellRules::DrainHealing(spell, damageDealt);
	if (drained > 0) {
		if (!casterIsPlayer && bestiary)
			bestiary->RecordBehaviorObserved(caster.GetName(), "drain witnessed");
		const int hpBefore = caster.GetHP();
		caster.Heal(drained);
		const int actualHealing = caster.GetHP() - hpBefore;
		if (casterIsPlayer) stats.totalHealing += actualHealing;
		if (actualHealing > 0) Console::PrintSlow("  (Soul Drain restores "
			+ std::to_string(actualHealing) + " HP to " + caster.GetName() + ".)");
	}
}

static StatusTurnResult ProcessTurnStatuses(Entity& entity, bool isPlayer,
	GameStats& gameStats, Bestiary* bestiary) {
	Enemy* enemy = dynamic_cast<Enemy*>(&entity);
	const bool regenerationSuppressed = enemy && enemy->IsRegenerationSuppressed();
	const bool hadRegeneration = entity.GetStatuses().Has(StatusType::Regeneration);
	StatusTurnResult result = entity.ProcessStatusTurn(regenerationSuppressed);
	if (enemy) enemy->AdvanceRegenerationSuppression();
	if (regenerationSuppressed && hadRegeneration) {
		Console::PrintSlow("  Fire suppresses " + entity.GetName() + "'s regeneration.");
		if (enemy && bestiary) bestiary->RecordBehaviorObserved(enemy->GetName(),
			"regeneration suppressed by Fire");
	}
	if (result.poisonDamage > 0) {
		Console::PrintSlow("  Poison deals " + std::to_string(result.poisonDamage)
			+ " damage to " + entity.GetName() + ".");
	}
	if (result.burnDamage > 0) {
		Console::PrintSlow("  Burn deals " + std::to_string(result.burnDamage)
			+ " damage to " + entity.GetName() + ".");
	}
	if (result.bleedDamage > 0) {
		Console::PrintSlow("  ** BLEED RUPTURES for "
			+ std::to_string(result.bleedDamage) + " damage to "
			+ entity.GetName() + "! The wound meter empties. **");
	}
	if (result.regenerationHealing > 0) {
		Console::PrintSlow("  Regeneration restores "
			+ std::to_string(result.regenerationHealing) + " HP to "
			+ entity.GetName() + ".");
		if (enemy && bestiary) {
			bestiary->RecordStatusObserved(enemy->GetName(), StatusType::Regeneration);
			bestiary->RecordBehaviorObserved(enemy->GetName(),
				"regeneration witnessed");
		}
	}
	if (result.skipAction && entity.IsAlive()) {
		Console::PrintSlow("  " + entity.GetName() + " is frozen and loses this action!");
	}
	if (isPlayer) gameStats.totalDamageTaken += result.TotalDamage();
	else gameStats.totalDamageDealt += result.TotalDamage();
	return result;
}

bool CombatSystem::ResolveCombat(Player& player, std::vector<Enemy>& enemies,
	std::set<std::string>& seenEnemyTypes, GameStats& gameStats,
	Bestiary& bestiary) {
	if (enemies.empty()) return true;

	Console::Clear();
	CombatDisplay::PrintEncounterIntro(enemies);

	std::vector<EnemyKnowledge> knowledge(enemies.size(), EnemyKnowledge::None);
	for (size_t i = 0; i < enemies.size(); ++i) {
		Enemy& enemy = enemies[i];
		knowledge[i] = bestiary.GetKnowledge(enemy.GetName());
		if (knowledge[i] == EnemyKnowledge::None
			&& seenEnemyTypes.count(enemy.GetName())) {
			knowledge[i] = EnemyKnowledge::Approximate;
		}
		if (bestiary.RecordEncounter(enemy, knowledge[i], player.GetIntelligence())) {
			Console::PrintSlow("  ** New bestiary entry: " + enemy.GetName() + "! **");
		}
		knowledge[i] = bestiary.GetKnowledge(enemy.GetName());
	}

	// Turn loop
	bool deathSaveUsed = false;
	bool huntersLensAvailable = player.HasRelic(RelicId::HuntersLens);
	int roundNumber = 1;
	static RNG commitmentRng;

	while (player.IsAlive() && CountLivingEnemies(enemies) > 0) {
		const std::vector<int> initiativeOrder = BuildInitiativeOrder(player, enemies);
		int fastestEnemySpeed = 1;
		std::vector<int> enemyActions(enemies.size(), 0);
		std::vector<TurnAction> plannedActions(enemies.size());
		std::vector<bool> plannedIntentPending(enemies.size(), false);
		std::vector<bool> plannedCommitments(enemies.size(), false);
		std::vector<bool> weaknessKnown(enemies.size(), false);

		for (size_t i = 0; i < enemies.size(); ++i) {
			if (!enemies[i].IsAlive()) continue;
			fastestEnemySpeed = std::max(fastestEnemySpeed, enemies[i].GetSpeed());
			enemyActions[i] = enemies[i].ActionsPerRound(player.GetSpeed());
			plannedActions[i] = enemies[i].DecideTurn();
			plannedIntentPending[i] = true;
			plannedCommitments[i] = EnemyBehavior::RollEarlyCommitment(
				enemies[i], plannedActions[i], commitmentRng);
			weaknessKnown[i] = bestiary.IsWeaknessKnown(enemies[i].GetName());
		}
		const int playerActions = player.ActionsPerRound(fastestEnemySpeed);
		CombatRuntime runtime;

		CombatDisplay::PrintRoundHeader(roundNumber, player, enemies, knowledge,
			weaknessKnown, initiativeOrder, plannedActions, plannedIntentPending,
			plannedCommitments);

		auto redrawCombat = [&]() {
			for (size_t i = 0; i < enemies.size(); ++i) {
				weaknessKnown[i] = bestiary.IsWeaknessKnown(enemies[i].GetName());
			}
			CombatDisplay::PrintRoundHeader(roundNumber, player, enemies, knowledge,
				weaknessKnown, initiativeOrder, plannedActions, plannedIntentPending,
				plannedCommitments);
		};

		auto doPlayerActions = [&]() {
			for (int actionNumber = 0;
			actionNumber < playerActions && player.IsAlive()
				&& CountLivingEnemies(enemies) > 0; ++actionNumber) {
				if (playerActions > 1) {
					std::cout << "\n  [Player Action " << (actionNumber + 1)
						<< "/" << playerActions << "]\n";
				}
				player.SetDefending(false);
				const StatusTurnResult status = ProcessTurnStatuses(
					player, true, gameStats, &bestiary);
				if (!player.IsAlive()) AttemptDeathSave(player, deathSaveUsed);
				if (!player.IsAlive() || status.skipAction) continue;

				TurnAction action = CombatMenu::ChooseAction(player);
				int targetIndex = FirstLivingEnemy(enemies);
				if (ActionNeedsEnemyTarget(player, action)) {
					targetIndex = CombatMenu::ChooseTarget(enemies,
						action.type == ActionType::Inspect
							? "Which enemy do you inspect?"
							: "Choose a target:");
				}
				if (targetIndex < 0) {
					--actionNumber;
					continue;
				}

				Enemy& target = enemies[targetIndex];
				if (action.type == ActionType::Inspect) {
					knowledge[targetIndex] = InspectEnemy(
						player, target, knowledge[targetIndex]);
					bestiary.ImproveKnowledge(target, knowledge[targetIndex],
						player.GetIntelligence());
					for (size_t i = 0; i < enemies.size(); ++i) {
						if (enemies[i].GetName() == target.GetName()
							&& knowledge[i] < knowledge[targetIndex]) {
							knowledge[i] = knowledge[targetIndex];
						}
					}
					target.PrintStatus(knowledge[targetIndex]);
					if (plannedIntentPending[targetIndex]) {
						std::cout << "  Updated read:\n";
						CombatDisplay::PrintEnemyIntent(targetIndex, target,
							plannedActions[targetIndex], knowledge[targetIndex],
							player.GetIntelligence(), plannedCommitments[targetIndex]);
					}
					if (huntersLensAvailable) {
						huntersLensAvailable = false;
						--actionNumber;
						Console::PrintSlow("  (Hunter's Lens: this Inspect action was free.)");
					}
				}
				else if (action.type == ActionType::UseItem) {
					if (action.itemIndex == -2) {
						bestiary.Print();
						redrawCombat();
						--actionNumber;
					}
					else {
						int hp = player.GetHP();
						int mana = player.GetMana();
						player.GetInventory().UseItem(action.itemIndex, hp,
							player.GetMaxHP(), mana, player.GetMaxMana());
						if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
						if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
					}
				}
				else {
					std::vector<bool> wasAlive(enemies.size(), false);
					for (std::size_t i = 0; i < enemies.size(); ++i) {
						wasAlive[i] = enemies[i].IsAlive();
					}
					ExecuteAction(player, target, action, gameStats, &target, true,
						&bestiary, nullptr, &enemies);
					for (std::size_t i = 0; i < enemies.size(); ++i) {
						if (wasAlive[i] && !enemies[i].IsAlive()) {
							AnnounceEnemyDefeat(enemies, static_cast<int>(i));
							RecordEnemyDefeat(player, enemies[i], gameStats, bestiary);
						}
					}
					if (action.type == ActionType::Attack
						|| action.type == ActionType::CastSpell
						|| action.type == ActionType::Defend) {
						for (Enemy& observer : enemies) {
							if (observer.IsAlive()) observer.ObservePlayerAction(action);
						}
					}
					if (!player.IsAlive()) AttemptDeathSave(player, deathSaveUsed);
				}
			}
		};

		auto doEnemyActions = [&](int enemyIndex) {
			Enemy& enemy = enemies[enemyIndex];
			for (int actionNumber = 0;
				actionNumber < enemyActions[enemyIndex] && player.IsAlive()
				&& enemy.IsAlive(); ++actionNumber) {
				if (enemyActions[enemyIndex] > 1) {
					std::cout << "  [E" << (enemyIndex + 1) << " Action "
						<< (actionNumber + 1) << "/" << enemyActions[enemyIndex] << "]\n";
				}
				enemy.SetDefending(false);
				const bool enemyWasAlive = enemy.IsAlive();
				const StatusTurnResult status = ProcessTurnStatuses(
					enemy, false, gameStats, &bestiary);
				if (enemyWasAlive && !enemy.IsAlive()) {
					plannedIntentPending[enemyIndex] = false;
					AnnounceEnemyDefeat(enemies, enemyIndex);
					RecordEnemyDefeat(player, enemy, gameStats, bestiary);
					break;
				}
				if (status.skipAction) {
					if (actionNumber == 0) plannedIntentPending[enemyIndex] = false;
					continue;
				}

				TurnAction action;
				if (actionNumber == 0) {
					action = plannedActions[enemyIndex];
					plannedIntentPending[enemyIndex] = false;
					if (!plannedCommitments[enemyIndex]) {
						if (EnemyBehavior::ShouldReviseUncommittedPlan(enemy, commitmentRng)) {
							const TurnAction revised = enemy.DecideTurn();
							if (!EnemyBehavior::SameAction(action, revised)) {
								bestiary.RecordBehaviorObserved(enemy.GetName(), "feint witnessed");
								action = revised;
							}
						}
						plannedCommitments[enemyIndex] = true;
						if (action.type != ActionType::None) {
							CombatDisplay::PrintEnemyIntent(enemyIndex, enemy, action,
								knowledge[enemyIndex], player.GetIntelligence(), true);
						}
					}
				}
				else {
					action = enemy.DecideTurn();
					CombatDisplay::PrintEnemyIntent(enemyIndex, enemy, action,
						knowledge[enemyIndex], player.GetIntelligence(), true, true);
				}

				ExecuteAction(enemy, player, action, gameStats, nullptr, false,
					&bestiary, &runtime);
				if (enemyWasAlive && !enemy.IsAlive()) {
					AnnounceEnemyDefeat(enemies, enemyIndex);
					RecordEnemyDefeat(player, enemy, gameStats, bestiary);
				}
				if (!player.IsAlive()) AttemptDeathSave(player, deathSaveUsed);
				if (runtime.defenseQteOccurred && player.IsAlive()
					&& CountLivingEnemies(enemies) > 0) {
					runtime.defenseQteOccurred = false;
					redrawCombat();
				}
			}
		};

		for (int actor : initiativeOrder) {
			if (!player.IsAlive() || CountLivingEnemies(enemies) == 0) break;
			if (actor < 0) doPlayerActions();
			else if (enemies[actor].IsAlive()) doEnemyActions(actor);
		}

		if (player.IsAlive() && CountLivingEnemies(enemies) > 0) {
			Console::WaitForEnter();
			Console::Clear();
		}
		++roundNumber;
	}

	for (const Enemy& enemy : enemies) seenEnemyTypes.insert(enemy.GetName());

	if (player.IsAlive()) {
		Console::PrintSlow(enemies.size() == 1
			? "\n  ** You vanquish the " + enemies.front().GetName() + "! **"
			: "\n  ** The enemy group is broken. You survive the ambush! **");
		for (Enemy& enemy : enemies) {
			player.GainXP(enemy.GetXPReward());
			for (const Spell& spell : enemy.GetKnownSpells()) {
				player.TryLearnSpell(spell);
			}
		}
		Console::WaitForEnter();
		Console::Clear();
		return true;
	}

	Console::PrintSlow("\n  " + PickRandom({
		player.GetName() + " has fallen in combat...",
		player.GetName() + " collapses beneath the assault...",
		"Darkness closes in... " + player.GetName() + " is no more.",
	}));
	Console::WaitForEnter();
	Console::Clear();
	return false;
}
