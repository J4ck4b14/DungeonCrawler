// CombatSystem.cpp
// -----------------
// Resolves turn-based combat between the player and an enemy.
//
// Combat flow: actors resolve in initiative order each round.
// Player defense is reactive: commit to Defend, then catch falling key cues.
// A completed sequence blocks; an all-perfect sequence parries and counters.
//
//
// Attack style balance:
//   Slash:  1.0x ATK, 15% crit (1.5x). Reliable.
//   Thrust: 0.8x normally, 1.0x vs defenders. Ignores defense.
//   Bash:   1.3x ATK, 15% whiff + self-damage. High risk/reward.
//
// Reactive Defense:
//   ALL PERFECT: 0 damage and an automatic counter.
//   ALL CAUGHT:  Half damage.
//   ANY MISS:    Full damage.
// Enemy guards retain hidden directional stances.
//
// Enemy intent is committed at round start. Better knowledge and Intelligence
// turn a vague warning into a precise read of the selected action.

#include "CombatSystem.h"
#include "CombatRules.h"
#include "DefenseRules.h"
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

struct CombatRuntime {
	bool aegisCoilUsedThisRound = false;
	bool defenseQteOccurred = false;
};

static std::vector<char> GenerateDefenseKeys(int count) {
	static RNG rng;
	static constexpr char keys[] = {'W', 'A', 'S', 'D'};
	std::vector<char> result;
	result.reserve(static_cast<size_t>(std::max(0, count)));
	for (int i = 0; i < count; ++i) {
		result.push_back(keys[rng.NextInt(0, 3)]);
	}
	return result;
}

static bool ResolveReactiveDefense(Entity& attacker, Entity& target,
	const TurnAction& action, const Spell* spell, int damage,
	GameStats& stats, CombatRuntime* runtime) {
	if (!target.IsDefending()) return false;
	Player* player = dynamic_cast<Player*>(&target);
	if (!player) return false;

	const int cueCount = DefenseRules::CueCount(action, spell);
	const DefenseChallenge challenge = DefenseRules::BuildChallenge(
		action, spell, attacker.GetSpeed(), attacker.GetATK(), GenerateDefenseKeys(cueCount));
	const DefenseResult result = DefenseQTE::Run(challenge);
	if (runtime) runtime->defenseQteOccurred = true;

	if (result == DefenseResult::PerfectParry) {
		Console::PrintSlow("  ** PERFECT PARRY! No damage received. **");
		const int counter = action.type == ActionType::CastSpell
			? CombatRules::MagicCounterDamage(player->GetATK())
			: CombatRules::PhysicalCounterDamage(player->GetATK());
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
		player->ReceiveDamage(DefenseRules::DamageAfterDefense(damage, result));
		player->SetDefending(wasDefending);
		const int damageTaken = hpBefore - player->GetHP();
		stats.totalDamageTaken += damageTaken;
		Console::PrintSlow("  ** BLOCK! Damage reduced to "
			+ std::to_string(damageTaken) + ". **");
		ApplyOnHitRelics(attacker, *player, damageTaken, false);
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
	player->ReceiveDamage(DefenseRules::DamageAfterDefense(damage, result));
	player->SetDefending(wasDefending);
	const int damageTaken = hpBefore - player->GetHP();
	stats.totalDamageTaken += damageTaken;
	Console::PrintSlow("  ** GUARD BREAK! Full damage: "
		+ std::to_string(damageTaken) + ". **");
	ApplyOnHitRelics(attacker, *player, damageTaken, false);
	return true;
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

// ---- Execute a single action ----
// Now takes a Bestiary* so we can record weakness discoveries in combat

static void ExecuteAction(Entity& actor, Entity& target, const TurnAction& action,
	GameStats& stats, Enemy* enemyTarget, bool isPlayer, Bestiary* bestiary = nullptr,
	CombatRuntime* runtime = nullptr) {

	switch (action.type) {

	case ActionType::Attack: {
		static RNG rng;
		int baseAtk = actor.GetATK();
		int dmg = baseAtk;
		bool missed = false;
		bool crit = false;

		// -- Calculate damage based on style --
		switch (action.attackStyle) {
		case AttackStyle::Slash: {
			dmg = baseAtk;
			float critChance = CombatRules::SlashCritChance;
			// Lucky Coin: doubled crit chance for the player
			if (isPlayer && static_cast<Player&>(actor).HasRelic(RelicId::LuckyCoin))
				critChance = CombatRules::LuckyCoinCritChance;
			if (rng.Chance(critChance)) {
				dmg = static_cast<int>(baseAtk * CombatRules::SlashCritMultiplier);
				crit = true;
			}
			break;
		}
		case AttackStyle::Thrust:
			dmg = target.IsDefending()
				? baseAtk                              // Full damage vs defenders
				: static_cast<int>(baseAtk * CombatRules::ThrustDamageMultiplier);
			if (dmg < 1) dmg = 1;
			break;
		case AttackStyle::Bash:
			if (rng.Chance(CombatRules::BashMissChance)) {
				missed = true;
			} else {
				dmg = static_cast<int>(baseAtk * CombatRules::BashDamageMultiplier);
			}
			break;
		}

		// -- Apply sharpening buff --
		int buffBonus = missed ? 0 : actor.ConsumeAttackBuff(false);
		if (buffBonus > 0) {
			dmg += buffBonus;
			Console::PrintSlow("  (Sharpened weapon: +" + std::to_string(buffBonus) + " bonus!)");
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
			int selfDmg = std::max(1, static_cast<int>(baseAtk * CombatRules::BashRecoilMultiplier));
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

		const int unempoweredDamage = dmg;
		dmg = actor.ConsumePowerBuff(dmg);
		if (dmg > unempoweredDamage) {
			Console::PrintSlow("  (Empower surges: "
				+ std::to_string(dmg - unempoweredDamage) + " bonus damage!)");
		}

		// -- Hit message (varied per style) --
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
			dmg, stats, runtime)) {
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
				int counter = CombatRules::PhysicalCounterDamage(target.GetATK());
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
			}
			else {
				// Wrong guess vs Slash/Bash: halve damage, no counter
				int actualDmg = dmg / 2;
				target.ReceiveDamage(dmg); // ReceiveDamage halves internally
				Console::PrintSlow("  (Halved by defense!)");
				if (isPlayer) stats.totalDamageDealt += actualDmg;
				if (!isPlayer) stats.totalDamageTaken += actualDmg;
				ApplyOnHitRelics(actor, target, actualDmg, isPlayer);
			}
		}
		else {
			// Not defending: full damage
			target.ReceiveDamage(dmg);
			if (isPlayer) stats.totalDamageDealt += dmg;
			if (!isPlayer) stats.totalDamageTaken += dmg;
			ApplyOnHitRelics(actor, target, dmg, isPlayer);
		}
		break;
	}

	case ActionType::Defend: {
		actor.SetDefending(true);
		if (isPlayer) {
			Console::PrintSlow("  " + actor.GetName()
				+ " enters a reactive guard. Watch the timing line!");
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
		int manaCost = isPlayer
			? static_cast<Player&>(actor).GetEffectiveManaCost(spell)
			: spell.manaCost;
		actor.UseMana(manaCost);

		if (isPlayer) stats.RecordSpellCast(spell.name);

		if (spell.effect == SpellEffect::Damage) {
			int dmg = spell.power + actor.GetIntelligence() * 2;

			// Apply arcane study buff
			int buffBonus = actor.ConsumeAttackBuff(true);
			if (buffBonus > 0) {
				dmg += buffBonus;
				Console::PrintSlow("  (Arcane focus: +" + std::to_string(buffBonus) + " bonus!)");
			}
			const int unempoweredDamage = dmg;
			dmg = actor.ConsumePowerBuff(dmg);
			if (dmg > unempoweredDamage) {
				Console::PrintSlow("  (Empower surges: "
					+ std::to_string(dmg - unempoweredDamage) + " bonus damage!)");
			}

			// Weakness bonus
			bool hitWeakness = false;
			if (enemyTarget && spell.element == enemyTarget->GetWeakness()) {
				hitWeakness = true;
				dmg = static_cast<int>(dmg * CombatRules::WeaknessMultiplier);
				Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
					+ " [" + spell.GetElementName() + "] for " + std::to_string(dmg)
					+ " damage! It's super effective!");

				// Record weakness discovery in bestiary if player exploited it
				if (isPlayer && bestiary) {
					bool newDiscovery = bestiary->RecordWeaknessDiscovered(enemyTarget->GetName());
					if (newDiscovery) {
						Console::PrintSlow("  ** Weakness discovered: " + enemyTarget->GetName()
							+ " is weak to " + spell.GetElementName() + "! Added to bestiary! **");
					}
				}
			}
			else {
				Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
					+ " [" + spell.GetElementName() + "] for " + std::to_string(dmg) + " damage!");
			}

			if (!isPlayer && ResolveReactiveDefense(actor, target, action, &spell,
				dmg, stats, runtime)) {
				break;
			}

			bool spellDealtDamage = false;
			// Enemy guards retain their hidden elemental stance behavior.
			if (target.IsDefending() && target.GetDefenseStance() == DefenseStance::AntiMagic) {
				Console::PrintSlow("  " + PickRandom({
					"** MAGIC PARRY! " + target.GetName() + " deflects the spell! **",
					"** SPELL DEFLECTED! " + target.GetName() + " repels the magic! **",
					"** PARRY! " + target.GetName() + "'s ward absorbs the spell! **",
					"** MAGIC PARRY! The spell shatters against " + target.GetName() + "'s focus! **",
				}));
				int counter = CombatRules::MagicCounterDamage(target.GetATK());
				Console::PrintSlow("  " + PickRandom({
					target.GetName() + " retaliates for " + std::to_string(counter) + " damage!",
					target.GetName() + " channels the deflected energy back for " + std::to_string(counter) + " damage!",
					target.GetName() + " strikes back for " + std::to_string(counter) + " damage!",
				}));
				actor.ReceiveDamage(counter);
				if (isPlayer) stats.totalDamageTaken += counter;
				if (!isPlayer) stats.totalDamageDealt += counter;
				// Defender takes 0 damage
			}
			else if (target.IsDefending()) {
				// Wrong stance vs magic: halved
				target.ReceiveDamage(dmg); // ReceiveDamage halves internally
				spellDealtDamage = dmg / 2 > 0;
				Console::PrintSlow("  (Halved by defense!)");
				if (isPlayer) stats.totalDamageDealt += dmg / 2;
				if (!isPlayer) stats.totalDamageTaken += dmg / 2;
				if (!isPlayer) ApplyOnHitRelics(actor, target, dmg / 2, false);
			}
			else {
				target.ReceiveDamage(dmg);
				spellDealtDamage = dmg > 0;
				if (isPlayer) stats.totalDamageDealt += dmg;
				if (!isPlayer) stats.totalDamageTaken += dmg;
				if (!isPlayer) ApplyOnHitRelics(actor, target, dmg, false);
			}
			if (isPlayer && hitWeakness && spellDealtDamage
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
			// Healing spell
			int heal = spell.power + actor.GetIntelligence();
			actor.Heal(heal);
			if (isPlayer) stats.totalHealing += heal;
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and restores " + std::to_string(heal) + " HP!");
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
		&& spells[action.spellIndex].effect == SpellEffect::Damage;
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
		if (bestiary.RecordEnemy(enemy, knowledge[i], player.GetIntelligence())) {
			Console::PrintSlow("  ** New bestiary entry: " + enemy.GetName() + "! **");
			player.GainXP(3);
		}
	}

	// Turn loop
	bool deathSaveUsed = false;
	bool huntersLensAvailable = player.HasRelic(RelicId::HuntersLens);
	int roundNumber = 1;

	while (player.IsAlive() && CountLivingEnemies(enemies) > 0) {
		const std::vector<int> initiativeOrder = BuildInitiativeOrder(player, enemies);
		int fastestEnemySpeed = 1;
		std::vector<int> enemyActions(enemies.size(), 0);
		std::vector<TurnAction> plannedActions(enemies.size());
		std::vector<bool> plannedIntentPending(enemies.size(), false);
		std::vector<bool> weaknessKnown(enemies.size(), false);

		for (size_t i = 0; i < enemies.size(); ++i) {
			if (!enemies[i].IsAlive()) continue;
			fastestEnemySpeed = std::max(fastestEnemySpeed, enemies[i].GetSpeed());
			enemyActions[i] = enemies[i].ActionsPerRound(player.GetSpeed());
			plannedActions[i] = enemies[i].DecideTurn();
			plannedIntentPending[i] = true;
			weaknessKnown[i] = bestiary.IsWeaknessKnown(enemies[i].GetName());
		}
		const int playerActions = player.ActionsPerRound(fastestEnemySpeed);
		CombatRuntime runtime;

		CombatDisplay::PrintRoundHeader(roundNumber, player, enemies, knowledge,
			weaknessKnown, initiativeOrder, plannedActions, plannedIntentPending);

		auto redrawCombat = [&]() {
			for (size_t i = 0; i < enemies.size(); ++i) {
				weaknessKnown[i] = bestiary.IsWeaknessKnown(enemies[i].GetName());
			}
			CombatDisplay::PrintRoundHeader(roundNumber, player, enemies, knowledge,
				weaknessKnown, initiativeOrder, plannedActions, plannedIntentPending);
		};

		auto doPlayerActions = [&]() {
			for (int actionNumber = 0;
				actionNumber < playerActions && player.IsAlive()
				&& CountLivingEnemies(enemies) > 0; ++actionNumber) {
				if (playerActions > 1) {
					std::cout << "\n  [Player Action " << (actionNumber + 1)
						<< "/" << playerActions << "]\n";
				}

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
					player.SetDefending(false);
					knowledge[targetIndex] = InspectEnemy(
						player, target, knowledge[targetIndex]);
					bestiary.RecordEnemy(target, knowledge[targetIndex]);
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
							player.GetIntelligence());
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
						player.SetDefending(false);
						int hp = player.GetHP();
						int mana = player.GetMana();
						player.GetInventory().UseItem(action.itemIndex, hp,
							player.GetMaxHP(), mana, player.GetMaxMana());
						if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
						if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
					}
				}
				else {
					player.SetDefending(false);
					const bool targetWasAlive = target.IsAlive();
					ExecuteAction(player, target, action, gameStats, &target, true, &bestiary);
					if (targetWasAlive && !target.IsAlive()) {
						AnnounceEnemyDefeat(enemies, targetIndex);
						RecordEnemyDefeat(player, target, gameStats, bestiary);
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

				TurnAction action;
				if (actionNumber == 0) {
					action = plannedActions[enemyIndex];
					plannedIntentPending[enemyIndex] = false;
				}
				else {
					action = enemy.DecideTurn();
					CombatDisplay::PrintEnemyIntent(enemyIndex, enemy, action,
						knowledge[enemyIndex], player.GetIntelligence(), true);
				}

				const bool enemyWasAlive = enemy.IsAlive();
				enemy.SetDefending(false);
				ExecuteAction(enemy, player, action, gameStats, nullptr, false,
					nullptr, &runtime);
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
