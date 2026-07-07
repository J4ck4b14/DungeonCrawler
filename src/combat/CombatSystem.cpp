// CombatSystem.cpp
// -----------------
// Resolves turn-based combat between the player and an enemy.
//
// Combat flow: Player acts -> Enemy acts -> repeat.
// Defense is a PREDICTION — you choose your stance before the enemy moves,
// guessing what attack they'll use. If you guess right: PARRY (0 damage + counter).
//
//
// Attack style balance:
//   Slash:  1.0x ATK, 15% crit (1.5x). Reliable.
//   Thrust: 0.8x normally, 1.0x vs defenders. Ignores defense.
//   Bash:   1.3x ATK, 15% whiff + self-damage. High risk/reward.
//
// Directional Defense:
//   CORRECT GUESS (physical): PARRY! 0 damage, counter = 1.3x ATK.
//   CORRECT GUESS (magic):    PARRY! 0 damage, counter = 0.9x ATK.
//   WRONG GUESS:              Halves damage, no counter.
//
// Enemy stance and attack style are NEVER revealed to the player directly.

#include "CombatSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "core/GameStats.h"
#include "core/Bestiary.h"
#include "utils/RNG.h"
#include "utils/Console.h"
#include "core/Relic.h"
#include <iostream>
#include <algorithm>
#include <set>

// ---- Flavor text helpers ----

static std::string PickRandom(const std::vector<std::string>& lines) {
	static RNG rng;
	return lines[rng.NextInt(0, static_cast<int>(lines.size()) - 1)];
}

// ---- Parry check ----

static bool IsPhysicalParry(DefenseStance stance, AttackStyle style) {
	return (stance == DefenseStance::AntiSlash  && style == AttackStyle::Slash)
		|| (stance == DefenseStance::AntiThrust && style == AttackStyle::Thrust)
		|| (stance == DefenseStance::AntiBash   && style == AttackStyle::Bash);
}

// ---- On-hit relic effects (physical damage only) ----
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

// ---- Execute a single action ----
// Now takes a Bestiary* so we can record weakness discoveries in combat

static void ExecuteAction(Entity& actor, Entity& target, const TurnAction& action,
	GameStats& stats, Enemy* enemyTarget, bool isPlayer, Bestiary* bestiary = nullptr) {

	actor.SetDefending(false);

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
			float critChance = 0.15f;
			// Lucky Coin: doubled crit chance for the player
			if (isPlayer && static_cast<Player&>(actor).HasRelic(RelicId::LuckyCoin))
				critChance = 0.30f;
			if (rng.Chance(critChance)) {
				dmg = static_cast<int>(baseAtk * 1.5f);
				crit = true;
			}
			break;
		}
		case AttackStyle::Thrust:
			dmg = target.IsDefending()
				? baseAtk                              // Full damage vs defenders
				: static_cast<int>(baseAtk * 0.8f);   // Reduced otherwise
			if (dmg < 1) dmg = 1;
			break;
		case AttackStyle::Bash:
			if (rng.Chance(0.15f)) {
				missed = true;
			} else {
				dmg = static_cast<int>(baseAtk * 1.3f);
			}
			break;
		}

		// -- Apply sharpening buff --
		int buffBonus = actor.ConsumeAttackBuff(false);
		if (!missed && buffBonus > 0) {
			dmg += buffBonus;
			Console::PrintSlow("  (Sharpened weapon: +" + std::to_string(buffBonus) + " bonus!)");
		}

		if (isPlayer) stats.RecordPhysicalAttack();

		// Executioner's Edge: +50% physical damage vs enemies below 30% HP
		if (isPlayer && !missed && enemyTarget
			&& static_cast<Player&>(actor).HasRelic(RelicId::ExecutionersEdge)
			&& enemyTarget->GetHP() * 10 < enemyTarget->GetMaxHP() * 3) {
			dmg = static_cast<int>(dmg * 1.5f);
			Console::PrintSlow("  (Executioner's Edge: the wounded foe is exposed!)");
		}

		// -- Bash whiff --
		if (missed) {
			int selfDmg = std::max(1, static_cast<int>(baseAtk * 0.3f));
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

		if (target.IsDefending()) {
			if (IsPhysicalParry(target.GetDefenseStance(), action.attackStyle)) {
				// PARRY! Zero damage, full counter
				Console::PrintSlow("  " + PickRandom({
					"** PARRY! " + target.GetName() + " read the attack perfectly! **",
					"** PARRY! " + target.GetName() + " saw it coming and deflects! **",
					"** PERFECT BLOCK! " + target.GetName() + " turns the attack aside! **",
					"** PARRY! " + target.GetName() + " catches the blow and turns it! **",
				}));
				int counter = static_cast<int>(target.GetATK() * 1.3f);
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
		actor.SetDefenseStance(action.defenseStance);
		if (isPlayer) {
			// Player sees their own stance
			std::string stanceName;
			switch (action.defenseStance) {
			case DefenseStance::AntiSlash:  stanceName = "slashes"; break;
			case DefenseStance::AntiThrust: stanceName = "thrusts"; break;
			case DefenseStance::AntiBash:   stanceName = "heavy blows"; break;
			case DefenseStance::AntiMagic:  stanceName = "magic"; break;
			}
			Console::PrintSlow("  " + actor.GetName()
				+ " takes a defensive stance against " + stanceName + "!");
		}
		else {
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
		int manaCost = spell.manaCost;
		// Arcane Battery: spells cost 1 less mana (min 1)
		if (isPlayer && static_cast<Player&>(actor).HasRelic(RelicId::ArcaneBattery)) {
			manaCost = std::max(1, manaCost - 1);
		}
		actor.UseMana(manaCost);

		if (isPlayer) stats.RecordSpellCast(spell.name);

		if (spell.target == SpellTarget::Enemy) {
			int dmg = spell.power + actor.GetIntelligence() * 2;

			// Apply arcane study buff
			int buffBonus = actor.ConsumeAttackBuff(true);
			if (buffBonus > 0) {
				dmg += buffBonus;
				Console::PrintSlow("  (Arcane focus: +" + std::to_string(buffBonus) + " bonus!)");
			}

			// Weakness bonus
			bool hitWeakness = false;
			if (enemyTarget && spell.element == enemyTarget->GetWeakness()) {
				hitWeakness = true;
				dmg = static_cast<int>(dmg * 1.5f);
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

			// Check for magic parry
			if (target.IsDefending() && target.GetDefenseStance() == DefenseStance::AntiMagic) {
				Console::PrintSlow("  " + PickRandom({
					"** MAGIC PARRY! " + target.GetName() + " deflects the spell! **",
					"** SPELL DEFLECTED! " + target.GetName() + " repels the magic! **",
					"** PARRY! " + target.GetName() + "'s ward absorbs the spell! **",
					"** MAGIC PARRY! The spell shatters against " + target.GetName() + "'s focus! **",
				}));
				int counter = std::max(1, static_cast<int>(target.GetATK() * 0.9f));
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
				Console::PrintSlow("  (Halved by defense!)");
				if (isPlayer) stats.totalDamageDealt += dmg / 2;
				if (!isPlayer) stats.totalDamageTaken += dmg / 2;
			}
			else {
				target.ReceiveDamage(dmg);
				if (isPlayer) stats.totalDamageDealt += dmg;
				if (!isPlayer) stats.totalDamageTaken += dmg;
			}
		}
		else {
			// Healing spell
			int heal = spell.power + actor.GetIntelligence();
			actor.Heal(heal);
			if (isPlayer) stats.totalHealing += heal;
			Console::PrintSlow("  " + actor.GetName() + " casts " + spell.name
				+ " and restores " + std::to_string(heal) + " HP!");
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
		int reviveHp = std::max(1, player.GetMaxHP() / 15);
		player.Heal(reviveHp);
		player.IncrementDeathSave();

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

bool CombatSystem::ResolveCombat(Player& player, Enemy& enemy,
	std::set<std::string>& seenEnemyTypes, GameStats& gameStats,
	Bestiary& bestiary) {

	Console::Clear();

	// Initial knowledge from bestiary or floor sightings
	EnemyKnowledge knowledge = bestiary.GetKnowledge(enemy.GetName());
	if (knowledge == EnemyKnowledge::None && seenEnemyTypes.count(enemy.GetName())) {
		knowledge = EnemyKnowledge::Approximate;
	}

	Console::PrintSlow("\n==================================");
	Console::PrintSlow("  A " + enemy.GetName() + " appears!");
	Console::PrintSlow("==================================");

	enemy.PrintStatus(knowledge);

	// Show known weakness from bestiary if discovered (but not yet Full knowledge)
	if (knowledge != EnemyKnowledge::Full && bestiary.IsWeaknessKnown(enemy.GetName())) {
		Spell tmp;
		tmp.element = enemy.GetWeakness();
		std::cout << "  (Known weakness: " << tmp.GetElementName() << ")\n";
	}

	std::cout << "\n";

	// Bestiary discovery (capture player INT at discovery time)
	bool newDiscovery = bestiary.RecordEnemy(enemy, knowledge, player.GetIntelligence());
	if (newDiscovery) {
		Console::PrintSlow("  ** New bestiary entry: " + enemy.GetName() + "! **");
		player.GainXP(3);
	}

	// Turn loop
	bool deathSaveUsed = false;

	while (player.IsAlive() && enemy.IsAlive()) {
		int playerSpeed = player.GetSpeed();
		int enemySpeed = enemy.GetSpeed();
		int playerActions = player.ActionsPerRound(enemySpeed);
		int enemyActions = enemy.ActionsPerRound(playerSpeed);

		std::cout << "\n-- Round Start --\n";
		player.PrintStatus();
		enemy.PrintStatus(knowledge);

		// Show known weakness inline if discovered
		if (knowledge != EnemyKnowledge::Full && bestiary.IsWeaknessKnown(enemy.GetName())) {
			Spell tmp;
			tmp.element = enemy.GetWeakness();
			std::cout << "  (Known weakness: " << tmp.GetElementName() << ")\n";
		}

		bool playerFirst = playerSpeed >= enemySpeed;

		// -- Player actions --
		auto doPlayerActions = [&]() {
			for (int i = 0; i < playerActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (playerActions > 1)
					std::cout << "  [Action " << (i + 1) << "/" << playerActions << "]\n";

				TurnAction action = player.DecideTurn();

				if (action.type == ActionType::Inspect) {
					knowledge = InspectEnemy(player, enemy, knowledge);
					bestiary.RecordEnemy(enemy, knowledge);
					enemy.PrintStatus(knowledge);
				}
				else if (action.type == ActionType::UseItem) {
					if (action.itemIndex == -2) {
						// Bestiary view (free action)
						bestiary.Print();
						i--;
					}
					else {
						int hp = player.GetHP();
						int mana = player.GetMana();
						player.GetInventory().UseItem(
							action.itemIndex, hp, player.GetMaxHP(), mana, player.GetMaxMana());
						if (hp > player.GetHP()) player.Heal(hp - player.GetHP());
						if (mana > player.GetMana()) player.RestoreMana(mana - player.GetMana());
					}
				}
				else {
					ExecuteAction(player, enemy, action, gameStats, &enemy, true, &bestiary);
				}
			}
		};

		// -- Enemy actions --
		auto doEnemyActions = [&]() {
			for (int i = 0; i < enemyActions && player.IsAlive() && enemy.IsAlive(); ++i) {
				if (enemyActions > 1)
					std::cout << "  [Enemy Action " << (i + 1) << "/" << enemyActions << "]\n";

				TurnAction action = enemy.DecideTurn();
				ExecuteAction(enemy, player, action, gameStats, nullptr, false);

				// Death-save QTE: if the player just died, give them one chance
				if (!player.IsAlive()) {
					AttemptDeathSave(player, deathSaveUsed);
				}
			}
		};

		if (playerFirst) {
			doPlayerActions();
			if (enemy.IsAlive()) doEnemyActions();
			player.SetDefending(false);
			enemy.SetDefending(false);
		}
		else {
			doEnemyActions();
			bool playerWasDefending = player.IsDefending();
			if (player.IsAlive()) doPlayerActions();
			enemy.SetDefending(false);
			if (playerWasDefending) {
				player.SetDefending(false);
			}
		}

		// End of round: wait for input then clear
		if (player.IsAlive() && enemy.IsAlive()) {
			Console::WaitForEnter();
			Console::Clear();
		}
	}

	seenEnemyTypes.insert(enemy.GetName());

	if (player.IsAlive()) {
		Console::PrintSlow("\n  " + PickRandom({
			"** " + enemy.GetName() + " has been defeated! **",
			"** " + enemy.GetName() + " crumbles to the ground! **",
			"** " + enemy.GetName() + " falls! Victory! **",
			"** You vanquish the " + enemy.GetName() + "! **",
		}));
		gameStats.totalKills++;
		bestiary.RecordKill(enemy.GetName());

		player.GainXP(enemy.GetXPReward());

		for (const auto& spell : enemy.GetKnownSpells()) {
			player.TryLearnSpell(spell);
		}

		Console::WaitForEnter();
		Console::Clear();
		return true;
	}
	else {
		Console::PrintSlow("\n  " + PickRandom({
			player.GetName() + " has fallen in combat...",
			player.GetName() + " collapses to the floor...",
			"Darkness closes in... " + player.GetName() + " is no more.",
		}));
		Console::WaitForEnter();
		Console::Clear();
		return false;
	}
}
