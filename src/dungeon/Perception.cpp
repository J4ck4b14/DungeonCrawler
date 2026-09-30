// One hidden d20 + INT survey produces remembered hints for the four directions.
// Low rolls stay vague, strong rolls reveal real content, and a natural 1 lies
// confidently. The number itself is never shown, so players judge the wording
// rather than reverse-engineering the roll. Suspicious walls are only learned
// through a successful survey; generator truth is never exposed automatically.

#include "Perception.h"
#include "entities/Player.h"
#include "utils/RNG.h"
#include <iostream>
#include <sstream>
#include "core/DevMode.h"

namespace {

std::string RememberedOutcome(const Room& room) {
	switch (room.outcome) {
	case RoomOutcome::EnemyDefeated:
		return "the signs of your finished battle remain, but no enemy does";
	case RoomOutcome::ChestOpened:
		return "the chest you opened stands empty";
	case RoomOutcome::Rested:
		return "the alcove where you rested is quiet";
	case RoomOutcome::TrapTriggered:
		return "the sprung trap that caught you lies spent";
	case RoomOutcome::TrapDisarmed:
		return "the trap you discovered is safely disarmed";
	case RoomOutcome::EmptySearched:
		return "you already searched the empty chamber";
	case RoomOutcome::Unresolved:
		break;
	}

	switch (room.content) {
	case RoomContent::Combat: return "you already cleared the chamber";
	case RoomContent::Chest: return "you already dealt with the chest";
	case RoomContent::Rest: return "you already used the quiet alcove";
	case RoomContent::Trap: return "the room's trap is no longer a threat";
	case RoomContent::Empty: return "you already searched the empty chamber";
	case RoomContent::Staircase: return "the staircase still leads deeper";
	}
	return "you remember the chamber clearly";
}

std::string DescribeRememberedDirection(Direction direction, const Room& room) {
	return std::string("To the ") + DirectionName(direction) + ", "
		+ RememberedOutcome(room) + ".";
}

std::string DescribeCurrentRoom(const Room& room) {
	if (room.contentResolved) {
		return "  Here: " + RememberedOutcome(room) + ".";
	}
	if (room.content == RoomContent::Staircase) {
		return "  Here: the staircase waits for you to descend.";
	}
	return "  Here: this chamber has not been resolved.";
}

bool TryGetAdjacentRoom(Direction direction, const Room& currentRoom,
	const std::vector<std::vector<Room>>& grid, int gridSize,
	const Room*& adjacent) {
	int x = currentRoom.x;
	int y = currentRoom.y;
	switch (direction) {
	case Direction::North: --y; break;
	case Direction::East: ++x; break;
	case Direction::South: ++y; break;
	case Direction::West: --x; break;
	}
	if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) return false;
	adjacent = &grid[y][x];
	return adjacent->exists;
}

} // namespace

int Perception::Roll() {
	static RNG rng;
	int base = rng.NextInt(1, 20);
	// The test penalty cannot manufacture a natural 1 below the die's floor.
	if (DevMode::IsEnabled()) {
		base -= DevMode::GetPerceptionPenalty();
		if (base < 1) base = 1;
	}
	return base;
}

std::string Perception::DescribeWall(Direction dir, bool unusual,
	bool breakable, int toughness, int quality, int playerStrength) {
	const char* dirName = DirectionName(dir);

	if (unusual && !breakable) {
		return std::string("The wall to the ") + dirName
			+ " returns an unusual hollow echo, but its face is solid and unyielding.";
	}

	if (unusual && breakable) {
		std::string base = std::string("A fractured seam marks the wall to the ")
			+ dirName + ".";
		if (toughness >= 14) {
			base += " It looks sturdy despite the damage.";
		} else if (quality >= 3) {
			base += " The weakened stones appear breakable.";
		}
		if (quality >= 3 && playerStrength >= toughness) {
			base += " Your strength should be enough to force it.";
		}
		return base;
	}

	return std::string("To the ") + dirName
		+ ", a continuous wall of solid stone blocks the way.";
}

static RoomContent MisleadingContent(RoomContent actual) {
	static RNG rng;
	std::vector<RoomContent> lies;
	if (actual != RoomContent::Combat)    lies.push_back(RoomContent::Combat);
	if (actual != RoomContent::Chest)     lies.push_back(RoomContent::Chest);
	if (actual != RoomContent::Trap)      lies.push_back(RoomContent::Trap);
	if (actual != RoomContent::Rest)      lies.push_back(RoomContent::Rest);
	if (actual != RoomContent::Empty)     lies.push_back(RoomContent::Empty);
	// False staircase reads would create navigation promises the map cannot keep.
	return lies[rng.NextInt(0, static_cast<int>(lies.size()) - 1)];
}

// The same vocabulary is used for truthful reads and natural-1 lies so confidence
// in the prose does not reveal whether the underlying information is reliable.
static std::string ContentDescription(Direction dir, RoomContent content, int quality) {
	const char* dirName = DirectionName(dir);

	if (quality == 2) {
		std::string base = std::string("To the ") + dirName + ", ";
		switch (content) {
		case RoomContent::Combat:    return base + "you hear faint sounds of movement.";
		case RoomContent::Chest:     return base + "the air smells faintly of old wood and metal.";
		case RoomContent::Trap:      return base + "something feels... off. The floor looks uneven.";
		case RoomContent::Rest:      return base + "a calm stillness hangs in the air.";
		case RoomContent::Staircase: return base + "a faint draft rises from below.";
		case RoomContent::Empty:     return base + "it seems quiet. Perhaps empty.";
		}
	}

	if (quality == 3) {
		std::string base = std::string("To the ") + dirName + ", ";
		switch (content) {
		case RoomContent::Combat:    return base + "you hear growling. Something alive lurks there.";
		case RoomContent::Chest:     return base + "you catch a glint of something in the shadows -- could be treasure.";
		case RoomContent::Trap:      return base + "the stonework looks deliberately loose. Probably a trap.";
		case RoomContent::Rest:      return base + "there's a quiet alcove -- looks safe to rest.";
		case RoomContent::Staircase: return base + "a cold breeze rises from a descending staircase.";
		case RoomContent::Empty:     return base + "there seems to be nothing. Not alive, at least.";
		}
	}

	if (quality == 4) {
		std::string base = std::string("To the ") + dirName + ", ";
		switch (content) {
		case RoomContent::Combat:    return base + "a creature waits in ambush -- you can see its silhouette clearly.";
		case RoomContent::Chest:     return base + "a chest sits against the far wall, undisturbed.";
		case RoomContent::Trap:      return base + "a trap mechanism is visible in the floor -- easily avoidable if you're careful.";
		case RoomContent::Rest:      return base + "a safe alcove with a small spring -- perfect for resting.";
		case RoomContent::Staircase: return base + "stone steps spiral downward into the next floor of the dungeon.";
		case RoomContent::Empty:     return base + "an empty chamber. Nothing of interest.";
		}
	}

	std::string base = std::string("To the ") + dirName + ", ";
	switch (content) {
	case RoomContent::Combat:
		return base + "you sense every detail: a creature breathes in the dark, coiled and ready. "
			"You can almost feel its heartbeat. It hasn't noticed you yet.";
	case RoomContent::Chest:
		return base + "a treasure chest rests undisturbed. The lock is old and weak -- "
			"you can tell it will open easily. The contents feel... promising.";
	case RoomContent::Trap:
		return base + "the floor is rigged. You can trace the pressure plate, the tripwire, "
			"the mechanism. Walking through will be trivial now.";
	case RoomContent::Rest:
		return base + "a hidden alcove glows with faint warmth. Clean water trickles from the stone. "
			"It's as safe as anywhere in this dungeon.";
	case RoomContent::Staircase:
		return base + "stone steps descend in a perfect spiral. The air from below is colder, heavier. "
			"The next floor awaits.";
	case RoomContent::Empty:
		return base + "absolutely nothing. The room is bare -- no threats, no rewards, no secrets. Just dust.";
	}

	return "You sense nothing.";
}

std::string Perception::DescribeDirection(Direction dir, const Room& adjacent,
	int rollQuality) {
	static RNG rng;
	const char* dirName = DirectionName(dir);
	if (adjacent.visited && adjacent.contentResolved) {
		return DescribeRememberedDirection(dir, adjacent);
	}

	// rollQuality: -1 = nat 1 (misleading), 0 = terrible, 1 = poor,
	//              2 = okay, 3 = good, 4 = great, 5 = nat20

	if (rollQuality == -1) {
		RoomContent fakeContent = MisleadingContent(adjacent.content);
		return ContentDescription(dir, fakeContent, 3);
	}

	if (rollQuality <= 0) {
		std::vector<std::string> bad = {
			std::string("To the ") + dirName + ", you hear... something? Maybe? Hard to tell.",
			std::string("You squint ") + dirName + " but see only darkness.",
			std::string("The passage ") + dirName + " exists, but you sense nothing useful.",
		};
		return bad[rng.NextInt(0, static_cast<int>(bad.size()) - 1)];
	}

	if (rollQuality == 1) {
		std::string base = std::string("A passage leads ") + dirName + ". ";
		if (adjacent.visited) {
			return base + "You've been there before.";
		}
		return base + "You can't make out much.";
	}

	return ContentDescription(dir, adjacent.content, rollQuality);
}

void Perception::PerceiveFromRoom(Room& currentRoom,
	const std::vector<std::vector<Room>>& grid,
	int gridSize, const Player& player) {
	std::cout << "\n" << DescribeCurrentRoom(currentRoom) << "\n";

	if (currentRoom.perceptionUsed) {
		std::cout << "  You've already surveyed this room.\n";
		if (!currentRoom.hints.empty()) {
			std::cout << "\n  You recall:\n";
			for (const auto& hint : currentRoom.hints) {
				const Room* adjacent = nullptr;
				if (TryGetAdjacentRoom(hint.direction, currentRoom, grid, gridSize, adjacent)
					&& adjacent->visited && adjacent->contentResolved) {
					std::cout << "    "
						<< DescribeRememberedDirection(hint.direction, *adjacent) << "\n";
				}
				else {
					std::cout << "    " << hint.description << "\n";
				}
			}
		}
		return;
	}

	currentRoom.perceptionUsed = true;

	int rawRoll = Roll();
	int total = rawRoll + player.GetIntelligence();

	int quality;
	if (rawRoll == 1) quality = -1;           // Nat 1: misleading!
	else if (rawRoll == 20) quality = 5;      // Nat 20: omniscient
	else if (total <= 5) quality = 0;         // Very bad
	else if (total <= 10) quality = 1;        // Poor
	else if (total <= 15) quality = 2;        // Medium
	else if (total <= 19) quality = 3;        // Good
	else quality = 4;                         // Great (total 20+ without nat 20)

	if (rawRoll == 20) {
		std::cout << "\n  Your senses sharpen to a razor's edge. The dungeon reveals itself.\n\n";
	}
	else if (rawRoll == 1) {
		std::cout << "\n  You focus intently and get a clear read on your surroundings.\n\n";
	}
	else if (quality >= 3) {
		std::cout << "\n  You take a moment to survey your surroundings and pick up on details.\n\n";
	}
	else if (quality >= 1) {
		std::cout << "\n  You try to get a sense of your surroundings...\n\n";
	}
	else {
		std::cout << "\n  You strain your senses but the dungeon is hard to read.\n\n";
	}

	for (int d = 0; d < 4; ++d) {
		Direction dir = static_cast<Direction>(d);

		if (!currentRoom.HasExit(dir)) {
			int nx = currentRoom.x;
			int ny = currentRoom.y;
			switch (dir) {
			case Direction::North: ny--; break;
			case Direction::South: ny++; break;
			case Direction::East:  nx++; break;
			case Direction::West:  nx--; break;
			}

			bool inBounds = !(nx < 0 || nx >= gridSize || ny < 0 || ny >= gridSize);
			bool hasHidden = false;
			bool breakable = false;
			int toughness = 0;
			if (inBounds) {
				// Hidden-wall metadata is mirrored between neighboring rooms; accept either
				// side so perception remains robust if a caller inspects one side directly.
				if (currentRoom.HasHiddenExit(dir)) {
					hasHidden = true;
					const HiddenWall& wall = currentRoom.GetHiddenWall(dir);
					breakable = wall.breakable;
					toughness = wall.toughness;
				}
				else {
					const Room& adj = grid[ny][nx];
					if (adj.HasHiddenExit(OppositeDirection(dir))) {
						hasHidden = true;
						const HiddenWall& wall = adj.GetHiddenWall(OppositeDirection(dir));
						breakable = wall.breakable;
						toughness = wall.toughness;
					}
				}
			}

			// Generator truth is not a clue. Only a successful deliberate survey
			// changes the wall from UNKNOWN to SUSPECTED, without revealing geometry.
			const bool detected = hasHidden && quality >= 2;
			if (detected) currentRoom.SetWallKnowledge(dir, WallKnowledge::Suspected);
			std::string desc = DescribeWall(dir, detected,
				detected && breakable, toughness, quality, player.GetStrength());
			std::cout << "    " << desc << "\n";
			continue;
		}

		int nx = currentRoom.x;
		int ny = currentRoom.y;
		switch (dir) {
		case Direction::North: ny--; break;
		case Direction::South: ny++; break;
		case Direction::East:  nx++; break;
		case Direction::West:  nx--; break;
		}

		if (nx < 0 || nx >= gridSize || ny < 0 || ny >= gridSize) {
			std::string desc = DescribeWall(dir);
			std::cout << "    " << desc << "\n";
			continue;
		}

		const Room& adjacent = grid[ny][nx];
		std::string desc = DescribeDirection(dir, adjacent, quality);
		std::cout << "    " << desc << "\n";

		PerceptionHint hint;
		hint.direction = dir;
		hint.description = desc;
		// Every description is remembered; only strong truthful reads unlock the
		// mechanical knowledge used by room resolution (for example, known traps).
		hint.revealsContent = (quality >= 3);
		if (hint.revealsContent) {
			hint.revealedContent = adjacent.content;
		}
		currentRoom.hints.push_back(hint);
	}

	std::cout << "\n";
}
