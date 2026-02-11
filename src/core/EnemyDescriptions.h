// EnemyDescriptions.h
// -------------------
// Provides bestiary flavor text that scales with the player's Intelligence
// at the moment the creature was first discovered.
//
// The intent is: low INT -> blunt / caveman notes.
// High INT -> academic observations & usable combat remarks.

#pragma once
#include <string>
#include <map>
#include <vector>

namespace EnemyDescriptions {

enum class DetailTier {
	Primitive, // INT 0
	Plain,     // INT 1
	Keen,      // INT 2-3
	Scholarly  // INT 4+
};

inline DetailTier TierFromInt(int playerInt) {
	if (playerInt <= 0) return DetailTier::Primitive;
	if (playerInt == 1) return DetailTier::Plain;
	if (playerInt <= 3) return DetailTier::Keen;
	return DetailTier::Scholarly;
}

inline std::string GetDescription(const std::string& enemyName, int playerIntAtDiscovery) {
	const DetailTier tier = TierFromInt(playerIntAtDiscovery);

	// Each enemy gets 4 tiers. If missing, fall back to generic text.
	static const std::map<std::string, std::vector<std::string>> perEnemy = {
		{"Slime", {
			"Green blob. Jiggly. Hit until stop moving.",
			"A quivering ooze. It lashes out in short, sticky bursts.",
			"Amorphous protoplasm; slow to react but hard to read. Keep distance when it swells.",
			"A corrosive gelatinous organism. Observe its rhythmic compression—strike after it expels mass; fire expedites coagulation."
		}},
		{"Rat", {
			"Big rat. Bite. Fast. Don't let it bite.",
			"A cavern rat—quick, skittish, and eager to chew ankles.",
			"Nervous but opportunistic. Watch for darting feints; punish the retreat with a decisive hit.",
			"A territorial rodent with frantic burst movement. It commits fully once engaged—parry the lunge, then counter while it recovers."
		}},
		{"Skeleton", {
			"Bone man. No meat. Still mean.",
			"An animated skeleton. Its strikes are stiff, but it doesn't tire.",
			"Predictable patterns—reads like a metronome. Bait the swing, then respond.",
			"Reanimated osseous construct. Limited joint articulation yields telegraphed arcs; exploit timing windows and disrupt with focused magic."
		}},
		{"Spider", {
			"Many legs. Creepy. Smash it.",
			"A dungeon spider that moves in short, sudden skitters.",
			"It tests range before committing. Deny space—one clean hit ends the dance.",
			"An ambush-oriented arachnid. It favors lateral repositioning and quick probes; hold a stable guard, then punish overextension."
		}},
		{"Goblin", {
			"Small green guy. Stab. Loud.",
			"A goblin raider with nasty tricks and sudden courage.",
			"Cunning for its size—expects you to flinch. Stay calm; it folds under pressure.",
			"A scavenger-combatant with opportunistic aggression. Do not trade blows—force it into defense, then break tempo with magic."
		}},
		{"Bandit", {
			"Human thief. Wants my stuff.",
			"A bandit with practiced cruelty. Fights like they've done this before.",
			"Reads your posture and punishes hesitation. Feint, then strike true.",
			"A disciplined opportunist. Expect stance changes and targeted counters; deny them initiative with decisive openings and controlled defense."
		}},
		{"Orc", {
			"Big angry. Hits hard.",
			"A brutal orc that favors raw force over finesse.",
			"Slow to adapt, but a single mistake hurts. Brace, then answer with precision.",
			"A strength-driven brawler. Its commitment is absolute—let it spend momentum, then exploit recovery with thrusts or focused spells."
		}},
		{"Ghost", {
			"Cold air. Not normal. Bad.",
			"A restless spirit—its presence chills the room and your thoughts.",
			"Hard to read, but it hesitates before striking. Use that pause.",
			"An ethereal entity with inconsistent physical tells. Favor magic and maintain composure; its attacks are preceded by a distinct stillness."
		}},
		{"Witch", {
			"Old magic lady. Spells hurt.",
			"A witch who mixes curses and bursts of elemental power.",
			"Her casting has rhythm. Break it—pressure her before she completes a volley.",
			"A practiced spellcaster. Interrupt cadence: force defensive decisions, then punish mana expenditure with sustained aggression."
		}},
		{"Troll", {
			"Huge. Smells. Tough.",
			"A troll—slow, stubborn, and terrifying up close.",
			"Its swings are wide and costly. Survive the first, then punish the second.",
			"A heavy-mass predator. It relies on sweeping commitment; bait the strike, evade by stance, then retaliate before it re-centers."
		}},
		{"Werewolf", {
			"Wolf man. Teeth.",
			"A feral werewolf that attacks in frantic bursts.",
			"Unstable tempo—fast then suddenly still. Defend during the rush; strike in the quiet.",
			"A volatile apex hunter. Tracks openings instinctively; steady defense and punish after its burst sequence ends."
		}},
		{"Vampire", {
			"Pale. Smiling. Bad.",
			"A vampire with unsettling grace. It fights like a duelist.",
			"It wants you to overreach. Don't. Make it work for every inch.",
			"A predatory aristocrat. It excels at punishing impatience; maintain disciplined spacing and conserve resources for the decisive exchange."
		}},
		{"Dark Mage", {
			"Scary robe. Magic.",
			"A dark mage channeling multiple elements with practiced conviction.",
			"Versatile and dangerous. Anticipate spell variety—defense stance matters.",
			"A versatile arcane combatant. Expect elemental cycling; counter by reading mana usage and selecting anti-magic defense at high-risk moments."
		}},
		{"Demon", {
			"Horn thing. Very bad.",
			"A demon—malice made flesh. The air feels wrong around it.",
			"Aggression is its language. Survive the first assault; it bleeds momentum after.",
			"A hostile outsider with violent initiative. It escalates quickly—prioritize tempo control, defensive prediction, and resource-efficient damage."
		}},
		{"Giant", {
			"Too big. Steps shake room.",
			"A giant whose movements are slow but catastrophic.",
			"Telegraphed force. Let it swing; answer with speed and spells.",
			"A high-mass opponent with long recovery windows. Favor precise strikes and sustained magic; do not remain in its threat radius."
		}},
		{"Dragon", {
			"Dragon. Run?",
			"A dragon—ancient power with teeth and flame.",
			"It controls the fight. You must choose moments carefully and commit hard.",
			"A pinnacle predator. Its offense dictates the battlefield—prioritize elemental exploitation, stabilize against spikes, and strike only on certainty."
		}},
	};

	auto it = perEnemy.find(enemyName);
	if (it == perEnemy.end() || it->second.size() < 4) {
		switch (tier) {
		case DetailTier::Primitive: return "Thing here. Dangerous.";
		case DetailTier::Plain: return "A hostile creature encountered in the dungeon.";
		case DetailTier::Keen: return "An enemy with recognizable patterns and exploitable openings.";
		case DetailTier::Scholarly: return "A documented dungeon entity. Study its tempo, resources, and weaknesses for optimal engagement.";
		}
	}

	const size_t idx = (tier == DetailTier::Primitive) ? 0
		: (tier == DetailTier::Plain) ? 1
		: (tier == DetailTier::Keen) ? 2
		: 3;

	return it->second[idx];
}

} // namespace EnemyDescriptions
