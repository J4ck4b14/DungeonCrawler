// Authored reactive-defense motifs by enemy species. Rhythm carries most of the
// identity; only a few species bend visibility or lane movement as a readable trick.

#include "DefensePatterns.h"

#include "Spell.h"

#include <algorithm>
#include <initializer_list>

namespace {

DefenseNote Note(char key, std::initializer_list<char> path = {},
	int arrivalOffsetMs = 0,
	DefenseCueVisibility visibility = DefenseCueVisibility::Normal,
	bool decoy = false) {
	DefenseNote note;
	note.key = key;
	note.lanePath = path.size() == 0
		? std::vector<char>{key} : std::vector<char>(path);
	note.arrivalOffsetMs = arrivalOffsetMs;
	note.visibility = visibility;
	note.decoy = decoy;
	return note;
}

DefenseCue Beat(std::initializer_list<DefenseNote> notes,
	int fallDurationMs, int gapAfterMs) {
	return DefenseCue{std::vector<DefenseNote>(notes), fallDurationMs, gapAfterMs};
}

char RotateLane(char lane, int amount) {
	static constexpr char lanes[] = {'A', 'W', 'S', 'D'};
	int index = 0;
	for (int i = 0; i < 4; ++i) if (lanes[i] == lane) index = i;
	return lanes[(index + amount) % 4];
}

void VaryCueLanes(DefenseChallenge& challenge, int variant) {
	// Vary each beat independently so the player learns an enemy's rhythm rather
	// than memorising one fixed A/W/S/D sequence. Notes inside a chord rotate
	// together, preserving their authored relationship and any special motion path.
	for (std::size_t cueIndex = 0; cueIndex < challenge.cues.size(); ++cueIndex) {
		const int amount = ((variant + static_cast<int>(cueIndex)) % 4 + 4) % 4;
		if (amount == 0) continue;
		for (DefenseNote& note : challenge.cues[cueIndex].notes) {
			note.key = RotateLane(note.key, amount);
			for (char& lane : note.lanePath) lane = RotateLane(lane, amount);
		}
	}
}

std::string AttackLabel(const TurnAction& action, const Spell* spell) {
	if (action.type == ActionType::CastSpell) {
		return spell ? spell->name : "Hostile spell";
	}
	if (action.type != ActionType::Attack) return "Incoming attack";
	switch (action.attackStyle) {
	case AttackStyle::Slash: return "Sweeping slash";
	case AttackStyle::Thrust: return "Driving thrust";
	case AttackStyle::Bash: return "Crushing bash";
	}
	return "Incoming attack";
}

void ApplyAttackShape(DefenseChallenge& challenge,
	const TurnAction& action, const Spell* spell) {
	if (action.type == ActionType::Attack) {
		for (DefenseCue& cue : challenge.cues) {
			if (action.attackStyle == AttackStyle::Thrust) {
				cue.fallDurationMs -= 70;
				cue.gapAfterMs = std::max(70, cue.gapAfterMs - 45);
			}
			else if (action.attackStyle == AttackStyle::Bash) {
				cue.fallDurationMs += 110;
				cue.gapAfterMs += 70;
			}
		}
	}
	else if (action.type == ActionType::CastSpell && spell) {
		const int spellPressure = std::min(100, spell->manaCost * 12);
		for (DefenseCue& cue : challenge.cues) cue.fallDurationMs -= spellPressure;
	}
}

void AddRankMotifs(DefenseChallenge& challenge, int tier,
	const DefenseCue& first, const DefenseCue& second,
	const DefenseCue& third) {
	if (tier >= 1) challenge.cues.push_back(first);
	if (tier >= 2) challenge.cues.push_back(second);
	if (tier >= 3) challenge.cues.push_back(third);
}

} // namespace

namespace DefensePatterns {

DefenseChallenge Build(EnemyArchetype archetype, int enemyRank,
	const TurnAction& action, const Spell* spell, int patternVariant) {
	DefenseChallenge challenge;
	challenge.attackLabel = AttackLabel(action, spell);
	const int tier = enemyRank >= 40 ? 3 : enemyRank >= 25 ? 2
		: enemyRank >= 10 ? 1 : 0;

	switch (archetype) {
	case EnemyArchetype::Slime:
		challenge.patternLabel = "Sticky, irregular descent";
		challenge.cues = {
			Beat({Note('A')}, 1570, 430),
			Beat({Note('S')}, 1390, 610)};
		AddRankMotifs(challenge, tier,
			Beat({Note('D')}, 1510, 330), Beat({Note('W')}, 1280, 540),
			Beat({Note('A'), Note('S')}, 1430, 420));
		break;

	case EnemyArchetype::Rat:
		challenge.patternLabel = "Sudden isolated darts";
		challenge.cues = {
			Beat({Note('D')}, 900, 145),
			Beat({Note('A')}, 760, 250)};
		AddRankMotifs(challenge, tier,
			Beat({Note('W')}, 720, 120), Beat({Note('D')}, 680, 180),
			Beat({Note('S')}, 650, 110));
		break;

	case EnemyArchetype::Skeleton:
		challenge.patternLabel = "Metronomic bone cadence";
		challenge.cues = {
			Beat({Note('A')}, 1190, 280), Beat({Note('W')}, 1190, 280)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A')}, 1190, 280), Beat({Note('W')}, 1190, 280),
			Beat({Note('A'), Note('W')}, 1240, 300));
		break;

	case EnemyArchetype::Spider:
		challenge.patternLabel = "Crossing web-lines";
		challenge.cues = {
			Beat({Note('S', {'A', 'W', 'S'})}, 1080, 180),
			Beat({Note('W', {'D', 'S', 'W'})}, 1010, 190)};
		AddRankMotifs(challenge, tier,
			Beat({Note('D', {'A', 'W', 'S', 'D'})}, 980, 145),
			Beat({Note('A', {'D', 'S', 'W', 'A'}), Note('S', {'A', 'W', 'S'}, 120)}, 1030, 160),
			Beat({Note('W', {'A', 'W'}), Note('D', {'S', 'D'})}, 940, 135));
		break;

	case EnemyArchetype::Goblin:
		challenge.patternLabel = "Dirty feints and uneven release";
		challenge.cues = {
			Beat({Note('D', {}, 0, DefenseCueVisibility::Late, true),
				Note('S', {}, 130, DefenseCueVisibility::Late)}, 1120, 390),
			Beat({Note('A')}, 870, 115)};
		AddRankMotifs(challenge, tier,
			Beat({Note('W', {}, 0, DefenseCueVisibility::Late, true), Note('D')}, 910, 300),
			Beat({Note('W'), Note('S', {}, 95)}, 900, 130),
			Beat({Note('A', {}, 0, DefenseCueVisibility::Late)}, 720, 260));
		break;

	case EnemyArchetype::Bandit:
		challenge.patternLabel = "Measured duelist's feint";
		challenge.cues = {
			Beat({Note('A', {}, 0, DefenseCueVisibility::Normal, true),
				Note('W', {}, 105)}, 1120, 270),
			Beat({Note('D')}, 980, 210)};
		AddRankMotifs(challenge, tier,
			Beat({Note('S')}, 930, 185), Beat({Note('A'), Note('D')}, 1050, 240),
			Beat({Note('W')}, 850, 170));
		break;

	case EnemyArchetype::Orc:
		challenge.patternLabel = "Committed impact and recovery";
		challenge.cues = {
			Beat({Note('S')}, 1480, 650), Beat({Note('D')}, 1320, 470)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A'), Note('S')}, 1450, 590),
			Beat({Note('W')}, 1210, 430), Beat({Note('A'), Note('D')}, 1350, 520));
		break;

	case EnemyArchetype::Ghost:
		challenge.patternLabel = "Intermittent spectral pulse";
		challenge.cues = {
			Beat({Note('W', {}, 0, DefenseCueVisibility::Flicker)}, 1260, 350),
			Beat({Note('D', {}, 0, DefenseCueVisibility::Flicker)}, 1130, 320)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A', {}, 0, DefenseCueVisibility::Flicker)}, 1070, 260),
			Beat({Note('W'), Note('S', {}, 0, DefenseCueVisibility::Flicker)}, 1170, 300),
			Beat({Note('D', {}, 0, DefenseCueVisibility::Flicker)}, 960, 240));
		break;

	case EnemyArchetype::Witch:
		challenge.patternLabel = "Serpentine casting cadence";
		challenge.cues = {
			Beat({Note('D', {'A', 'W', 'S', 'W', 'A', 'W', 'S', 'D'})}, 1370, 260),
			Beat({Note('A', {'D', 'S', 'W', 'S', 'D', 'S', 'W', 'A'})}, 1190, 220)};
		AddRankMotifs(challenge, tier,
			Beat({Note('S', {'A', 'W', 'S', 'D', 'S'})}, 1090, 190),
			Beat({Note('W', {'D', 'S', 'W'}), Note('D', {'A', 'W', 'S', 'D'}, 105)}, 1150, 210),
			Beat({Note('A', {'D', 'S', 'W', 'A'})}, 930, 170));
		break;

	case EnemyArchetype::Troll:
		challenge.patternLabel = "Broad sweeping pressure";
		challenge.cues = {
			Beat({Note('A'), Note('D', {}, 115)}, 1570, 410),
			Beat({Note('W'), Note('S', {}, 115)}, 1490, 430)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A'), Note('W', {}, 120), Note('S', {}, 240)}, 1460, 390),
			Beat({Note('D')}, 1320, 360), Beat({Note('A'), Note('D')}, 1410, 400));
		break;

	case EnemyArchetype::Werewolf:
		challenge.patternLabel = "Volatile hunting burst";
		challenge.cues = {
			Beat({Note('S')}, 820, 95), Beat({Note('A')}, 760, 75),
			Beat({Note('W')}, 700, 90), Beat({Note('D')}, 740, 310)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A')}, 680, 70), Beat({Note('S'), Note('D', {}, 90)}, 720, 105),
			Beat({Note('W')}, 640, 65));
		break;

	case EnemyArchetype::Vampire:
		challenge.patternLabel = "Precise dueling measure";
		challenge.cues = {
			Beat({Note('A')}, 1050, 210), Beat({Note('D')}, 1050, 210),
			Beat({Note('W'), Note('S')}, 1080, 250)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A')}, 990, 190), Beat({Note('D')}, 940, 180),
			Beat({Note('W'), Note('S')}, 1010, 210));
		break;

	case EnemyArchetype::DarkMage:
		challenge.patternLabel = "Sparse artificial groups";
		challenge.cues = {
			Beat({Note('A', {}, 0, DefenseCueVisibility::Sparse),
				Note('S', {}, 0, DefenseCueVisibility::Sparse)}, 1280, 430),
			Beat({Note('W', {}, 0, DefenseCueVisibility::Sparse)}, 1030, 390)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A', {}, 0, DefenseCueVisibility::Sparse),
				Note('D', {}, 0, DefenseCueVisibility::Sparse)}, 1120, 380),
			Beat({Note('W', {}, 0, DefenseCueVisibility::Sparse),
				Note('S', {}, 90, DefenseCueVisibility::Sparse)}, 1010, 350),
			Beat({Note('A', {}, 0, DefenseCueVisibility::Sparse),
				Note('W', {}, 0, DefenseCueVisibility::Sparse),
				Note('D', {}, 0, DefenseCueVisibility::Sparse)}, 1090, 400));
		break;

	case EnemyArchetype::Demon:
		challenge.patternLabel = "Escalating infernal momentum";
		challenge.cues = {
			Beat({Note('A')}, 1200, 240), Beat({Note('W')}, 1060, 190),
			Beat({Note('S')}, 920, 145)};
		AddRankMotifs(challenge, tier,
			Beat({Note('D')}, 820, 110), Beat({Note('A'), Note('S')}, 770, 90),
			Beat({Note('W'), Note('D')}, 700, 80));
		break;

	case EnemyArchetype::Giant:
		challenge.patternLabel = "Catastrophic clustered impact";
		challenge.cues = {
			Beat({Note('A'), Note('S'), Note('D')}, 1840, 720)};
		AddRankMotifs(challenge, tier,
			Beat({Note('W'), Note('D')}, 1720, 650),
			Beat({Note('A'), Note('W'), Note('S')}, 1650, 620),
			Beat({Note('A'), Note('W'), Note('S'), Note('D')}, 1760, 700));
		break;

	case EnemyArchetype::Dragon:
		challenge.patternLabel = "Layered apex motifs";
		challenge.cues = {
			Beat({Note('A', {'D', 'S', 'W', 'A'}), Note('S', {}, 105)}, 1300, 260),
			Beat({Note('W'), Note('D')}, 1160, 220),
			Beat({Note('S', {'A', 'W', 'S'}, 0, DefenseCueVisibility::Flicker)}, 1010, 190)};
		AddRankMotifs(challenge, tier,
			Beat({Note('A'), Note('D', {}, 100)}, 1050, 175),
			Beat({Note('W', {'D', 'S', 'W'}), Note('S')}, 940, 150),
			Beat({Note('A'), Note('W'), Note('S'), Note('D')}, 1120, 240));
		break;

	case EnemyArchetype::Count:
		break;
	}

	VaryCueLanes(challenge, patternVariant);
	ApplyAttackShape(challenge, action, spell);
	return challenge;
}

} // namespace DefensePatterns
