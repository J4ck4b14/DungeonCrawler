#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// Four field-note layers keyed by persistent Bestiary knowledge:
// immediate anatomy, temperament/cadence, mechanical counterplay, scholarship.
namespace EnemyDescriptions {

inline std::string GetDescription(const std::string& enemyName, int detailLevel) {
	detailLevel = std::clamp(detailLevel, 0, 3);
	static const std::map<std::string, std::vector<std::string>> notes = {
		{"Slime", {
			"A quivering mass that compresses before moving. Fire visibly stiffens it.",
			"It is slow, but releases its weight in sticky, irregular bursts.",
			"Do not copy a steady rhythm onto it. Watch the swell, and use Fire when available.",
			"The body stores force during compression, then expels it unevenly. Higher-Rank specimens endure longer, but remain slow and Fire-vulnerable."
		}},
		{"Rat", {
			"A lean dungeon rat with quick feet and very little protection.",
			"It darts at openings, sometimes retreating for a beat before lunging again.",
			"Isolated cues arrive quickly. Survive the dart and punish its low durability.",
			"Rank chiefly sharpens its speed. Its apparent hesitation is an opportunistic reset, not surrender."
		}},
		{"Skeleton", {
			"Rigid joints force every attack through the same narrow arcs.",
			"Its slash-slash-guard cadence repeats with metronomic discipline.",
			"Repetition makes it unusually learnable; anticipate the beat rather than reacting to personality.",
			"Animation preserves sequence but not judgment. Rank adds force and pattern length without teaching adaptation."
		}},
		{"Spider", {
			"A light, many-legged hunter whose fangs carry Poison.",
			"It probes, guards, and weaves laterally before committing.",
			"Track crossing lane movement and end the fight before Poison accumulation becomes costly.",
			"Higher Rank improves both mobility and Poison potency. The final lane, not the initial position, identifies its commitment."
		}},
		{"Goblin", {
			"A fragile scavenger that watches for fear and hesitation.",
			"It fakes retreats, hides releases, and alternates defense with dirty attacks.",
			"Do not answer the first visible motion automatically: some cues are deliberate decoys.",
			"Its timing remains uneven by design. Repeated player habits let it choose more useful guards, but composure exposes the trick."
		}},
		{"Bandit", {
			"A practiced human fighter with disciplined footwork.",
			"Measured feints conceal targeted thrusts and changing guard stances.",
			"It notices repeated attacks and can prepare the matching counter. Vary your approach.",
			"Unlike a monster, it adapts deliberately. Its false preparation is believable, but every committed line still has a readable finish."
		}},
		{"Orc", {
			"A dense, powerful brawler built for committed physical force.",
			"Heavy attacks are followed by conspicuous recovery pauses.",
			"Brace through the large impact, then spend the recovery window aggressively.",
			"Rank heavily increases Strength and Health, barely Speed. Once shoulder and feet commit, its intent becomes much easier to read."
		}},
		{"Ghost", {
			"An intermittent figure with little respect for ordinary physical form.",
			"It fades between attacks and produces strange, learnable pulses rather than random movement.",
			"Its cues flicker but return near the guard line. Arcane magic is especially effective.",
			"Physical tells remain poor even at high familiarity. Read the spectral pulse and exploit its Arcane weakness instead of chasing the silhouette."
		}},
		{"Witch", {
			"A ritual caster mixing Fire, Shadow, and restorative magic.",
			"Her casting follows a wicked cadence with deliberate silent beats.",
			"Cues travel serpentine across the lanes before dropping into their final commitment. Pressure can deny comfortable casting turns.",
			"The horizontal weave is preparation, not impact. Read the cadence, interrupt resource recovery, and commit only when the final lane resolves."
		}},
		{"Troll", {
			"A massive, slow creature whose wounds visibly knit themselves closed.",
			"Broad sweeps alternate with recovery while Regeneration sustains it.",
			"Fire temporarily suppresses Regeneration. Without it, a cautious fight favors the Troll.",
			"Rank increases regenerative stacks and heavy endurance far faster than speed. Apply Fire before using its long recovery as a damage window."
		}},
		{"Werewolf", {
			"A volatile predator poised for abrupt, very fast violence.",
			"Frantic attack bursts end in short periods of hunting silence.",
			"Recognize the rapid uneven sequence, defend through it, then act during the pause.",
			"Rank strongly favors Speed, producing more burst pressure without removing the recovery rhythm. Panic inputs make the sequence harder than it is."
		}},
		{"Vampire", {
			"A patient duelist that treats injury as an invitation rather than a setback.",
			"It guards, waits for overcommitment, and uses Soul Drain or healing to sustain itself.",
			"Its rhythm is precise. Avoid panic attacks and deny restorative casts when wounded.",
			"It studies repeated habits and converts predictable aggression into defense and sustain. Controlled variation is safer than constant pressure."
		}},
		{"Dark Mage", {
			"A physically weak caster surrounded by several elemental signatures.",
			"Its magical rhythm is sparse, artificial, and divided into discrete groups.",
			"Several cues may materialize together on alternating rows. Physical tells are especially unreliable.",
			"Rank flows primarily into Intelligence and mana. Track grouped spell cadence and force it to spend resources; do not expect martial readability."
		}},
		{"Demon", {
			"A violent initiator wreathed in heat. Its blows can inflict Burn.",
			"It alternates physical violence and magic while building oppressive momentum.",
			"Burn and accelerating defensive patterns are the central threats; losing tempo compounds both.",
			"Rank increases Strength, Intelligence, Burn reliability, and Burn potency. Break momentum early or escalating chords will coincide with attrition."
		}},
		{"Giant", {
			"Too big. The floor shakes well before impact.",
			"A Giant commits its whole body to each attack. Slow, but one mistake can end the fight.",
			"Its blows arrive as slow clustered chords. Readability is high; surviving an imperfect response is the problem.",
			"Shoulder and hip rotate well before impact. Giants are among the easiest threats to read, but Rank magnifies Health and Strength while preserving catastrophic group blows."
		}},
		{"Dragon", {
			"A pinnacle predator carrying physical and elemental danger at once.",
			"It deliberately combines movement, chords, breath magic, and restorative pressure.",
			"Expect Burn, Freeze-like disruption, chained Lightning, and mixed authored motifs. One defensive answer is never sufficient.",
			"High-Rank Dragons layer several readable threat families rather than attacking randomly. Survival demands status control, elemental judgment, precise guarding, and resource discipline together."
		}}
	};

	const auto found = notes.find(enemyName);
	if (found == notes.end()) {
		static const std::vector<std::string> fallback = {
			"An unfamiliar hostile shape.",
			"Repeated observation suggests a consistent temperament.",
			"Its cadence contains exploitable commitments.",
			"Field study confirms that Rank intensifies its established identity."
		};
		return fallback[static_cast<std::size_t>(detailLevel)];
	}
	return found->second[static_cast<std::size_t>(detailLevel)];
}

} // namespace EnemyDescriptions
