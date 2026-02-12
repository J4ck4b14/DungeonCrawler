// Perception.cpp
// ----------------
// Handles the player's perception checks when surveying a room.
// A hidden d20 + INT roll determines how much information the player
// receives about adjacent rooms:
//   Nat 1  -> Purposefully misleading: describes a WRONG room content
//             with confident, convincing detail so the player trusts it.
//   <=5    -> Vague / useless ("you see only darkness").
//   6-10   -> Poor: knows a passage exists, nothing more.
//   11-15  -> Medium: general vibe hints (sounds, smells, feelings).
//   16-19  -> Good: specific, truthful descriptions. Stored as canonical.
//   Nat 20 -> Full reveal with dramatic flavor text.
//
// The roll result is never shown to the player. Flavor text is chosen
// to mask the quality -- nat 1 sounds confident, low rolls sound uncertain.
// Canonical hints (quality >= 3) are stored on the Room so that traps
// detected via perception can be avoided when the player enters.
// Misleading hints from nat 1 are NOT stored as canonical reveals.

#include "Perception.h"
#include "entities/Player.h"
#include "utils/RNG.h"
#include <iostream>
#include <sstream>
#include "core/DevMode.h"

int Perception::Roll(const Player& player) {
	static RNG rng;
	int base = rng.NextInt(1, 20);
	// Apply dev-mode penalty (if enabled) to the raw d20. Minimum 1.
	if (DevMode::IsEnabled()) {
		base -= DevMode::GetPerceptionPenalty();
		if (base < 1) base = 1;
	}
	// Roll is hidden from the player -- no output here
	return base;
}

std::string Perception::DescribeWall(Direction dir, bool hasHidden, int toughness) {
	static RNG rng;
	const char* dirName = DirectionName(dir);

	// If there's a hidden/brittle wall here, prefer descriptions that hint weakness
	if (hasHidden) {
		std::vector<std::string> weakDescs = {
			std::string("To the ") + dirName + (", the masonry looks cracked and brittle."),
			std::string("The ") + dirName + (" wall has loose stones; it could be forced."),
			std::string("A seam in the wall to the ") + dirName + (" suggests it may yield to force."),
			std::string("The wall ") + dirName + (" is crumbly; a determined shove might open a passage.")
		};
		// If we want to subtly indicate toughness, append a hint for very high toughness
		std::string base = weakDescs[rng.NextInt(0, static_cast<int>(weakDescs.size()) - 1)];
		if (toughness >= 14) {
			base += " It looks especially sturdy, however.";
		} else if (toughness <= 8) {
			base += " It seems fragile enough for a strong shove.";
		}
		return base;
	}

	std::vector<std::string> wallDescs = {
		std::string("To the ") + dirName + (", there's a solid stone wall."),
		std::string("The ") + dirName + (" side is blocked by crumbling masonry."),
		std::string("A rough wall of rock blocks the way ") + dirName + ("."),
		std::string("To the ") + dirName + (", moss-covered bricks form an impassable barrier.")
	};
	return wallDescs[rng.NextInt(0, static_cast<int>(wallDescs.size()) - 1)];
}

// For nat-1 misleading: swap the content to something wrong
static RoomContent MisleadingContent(RoomContent actual) {
	static RNG rng;
	// Pick a different content type to lie about
	std::vector<RoomContent> lies;
	if (actual != RoomContent::Combat)    lies.push_back(RoomContent::Combat);
	if (actual != RoomContent::Chest)     lies.push_back(RoomContent::Chest);
	if (actual != RoomContent::Trap)      lies.push_back(RoomContent::Trap);
	if (actual != RoomContent::Rest)      lies.push_back(RoomContent::Rest);
	if (actual != RoomContent::Empty)     lies.push_back(RoomContent::Empty);
	// Intentionally don't fake staircases -- too game-breaking
	return lies[rng.NextInt(0, static_cast<int>(lies.size()) - 1)];
}

// Generate a description for a given content type (used for both truthful and misleading)
static std::string ContentDescription(Direction dir, RoomContent content, int quality) {
	const char* dirName = DirectionName(dir);

	if (quality == 2) {
		// Medium -- sense the general vibe
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
		// Good -- more specific
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
		// Great -- full reveal
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

	// quality >= 5 (nat 20) -- omniscient, atmospheric, almost narrative
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

	// rollQuality: -1 = nat 1 (misleading), 0 = terrible, 1 = poor,
	//              2 = okay, 3 = good, 4 = great, 5 = nat20

	if (rollQuality == -1) {
		// Nat 1: purposefully misleading -- describe a WRONG content with confidence
		RoomContent fakeContent = MisleadingContent(adjacent.content);
		// Use quality 3 (good) descriptions so the lie sounds convincing
		return ContentDescription(dir, fakeContent, 3);
	}

	if (rollQuality <= 0) {
		// Very low -- vague/useless
		std::vector<std::string> bad = {
			std::string("To the ") + dirName + ", you hear... something? Maybe? Hard to tell.",
			std::string("You squint ") + dirName + " but see only darkness.",
			std::string("The passage ") + dirName + " exists, but you sense nothing useful.",
		};
		return bad[rng.NextInt(0, static_cast<int>(bad.size()) - 1)];
	}

	if (rollQuality == 1) {
		// Low roll -- very vague
		std::string base = std::string("A passage leads ") + dirName + ". ";
		if (adjacent.visited) {
			return base + "You've been there before.";
		}
		return base + "You can't make out much.";
	}

	// Quality 2+ uses the content description helper (truthful)
	return ContentDescription(dir, adjacent.content, rollQuality);
}

void Perception::PerceiveFromRoom(Room& currentRoom,
	const std::vector<std::vector<Room>>& grid,
	int gridSize, const Player& player) {

	if (currentRoom.perceptionUsed) {
		std::cout << "  You've already surveyed this room.\n";
		// Re-display previous hints
		if (!currentRoom.hints.empty()) {
			std::cout << "\n  You recall:\n";
			for (const auto& hint : currentRoom.hints) {
				std::cout << "    " << hint.description << "\n";
			}
		}
		return;
	}

	currentRoom.perceptionUsed = true;

	int rawRoll = Roll(player);
	int total = rawRoll + player.GetIntelligence();

	// Determine quality tier -- roll is HIDDEN from the player
	int quality;
	if (rawRoll == 1) quality = -1;           // Nat 1: misleading!
	else if (rawRoll == 20) quality = 5;      // Nat 20: omniscient
	else if (total <= 5) quality = 0;         // Very bad
	else if (total <= 10) quality = 1;        // Poor
	else if (total <= 15) quality = 2;        // Medium
	else if (total <= 19) quality = 3;        // Good
	else quality = 4;                         // Great (total 20+ without nat 20)

	// Flavor text -- no numbers revealed
	if (rawRoll == 20) {
		std::cout << "\n  Your senses sharpen to a razor's edge. The dungeon reveals itself.\n\n";
	}
	else if (rawRoll == 1) {
		// Say something confident so the player trusts the lies
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

	// Check each direction
	for (int d = 0; d < 4; ++d) {
		Direction dir = static_cast<Direction>(d);

		if (!currentRoom.HasExit(dir)) {
			// Wall -- we may be looking at a hidden/breakable wall
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
			int toughness = 0;
			if (inBounds) {
				// Check either the current room's hidden flag in that direction (preferred)
				// or the adjacent room's opposite hidden flag (symmetry).
				if (currentRoom.HasHiddenExit(dir)) {
					hasHidden = true;
					toughness = currentRoom.GetHiddenToughness(dir);
				}
				else {
					const Room& adj = grid[ny][nx];
					if (adj.HasHiddenExit(OppositeDirection(dir))) {
						hasHidden = true;
						toughness = adj.GetHiddenToughness(OppositeDirection(dir));
					}
				}
			}

			std::string desc = DescribeWall(dir, hasHidden, toughness);
			std::cout << "    " << desc << "\n";
			continue;
		}

		// Calculate adjacent coords
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

		// Store as canonical hint
		PerceptionHint hint;
		hint.direction = dir;
		hint.description = desc;
		// Only good+ truthful rolls actually reveal content
		// Nat 1 misleading hints should NOT count as revealing (they're lies!)
		hint.revealsContent = (quality >= 3);
		if (hint.revealsContent) {
			hint.revealedContent = adjacent.content;
		}
		currentRoom.hints.push_back(hint);
	}

	std::cout << "\n";
}
