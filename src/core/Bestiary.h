// Bestiary.h
// ----------
// Persistent record of all enemy types the player has encountered.
// Each new discovery grants bonus XP. The bestiary stores the best
// knowledge level achieved for each enemy (from Inspect rolls).
 // Weakness can also be discovered by exploiting it in combat.
// Can be viewed during combat or exploration.

#pragma once
#include "entities/Enemy.h"
#include "combat/Spell.h"
#include "core/EnemyDescriptions.h"
#include "utils/Console.h"
#include "utils/AsciiArt.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>

// Snapshot of an enemy's stats at time of encounter
struct BestiaryEntry {
	std::string name;
	int maxHp = 0;
	int atk = 0;
	int speed = 0;
	int intelligence = 0;
	SpellElement weakness = SpellElement::Arcane;
	std::vector<std::string> spellNames;
	EnemyKnowledge bestKnowledge = EnemyKnowledge::None;
	int timesDefeated = 0;
	bool weaknessDiscovered = false; // Set when player exploits the weakness in combat

	int discoveredByIntelligence = 0; // Player INT when the entry was first discovered
	std::string description;          // Flavor text captured at discovery time
};

class Bestiary {
public:
	// Record an enemy encounter. Returns true if this is a NEW discovery.
	// discovererInt is the player's INT at the moment this entry is created.
	bool RecordEnemy(const Enemy& enemy, EnemyKnowledge knowledge, int discovererInt = 0) {
		const std::string& name = enemy.GetName();
		auto it = entries_.find(name);
		if (it == entries_.end()) {
			// New entry
			BestiaryEntry entry;
			entry.name = name;
			entry.maxHp = enemy.GetMaxHP();
			entry.atk = enemy.GetATK();
			entry.speed = enemy.GetSpeed();
			entry.intelligence = enemy.GetIntelligence();
			entry.weakness = enemy.GetWeakness();
			for (const auto& s : enemy.GetKnownSpells()) {
				entry.spellNames.push_back(s.name);
			}
			entry.bestKnowledge = knowledge;
			entry.timesDefeated = 0;
			entry.weaknessDiscovered = false;

			entry.discoveredByIntelligence = discovererInt;
			entry.description = EnemyDescriptions::GetDescription(entry.name, discovererInt);

			entries_[name] = entry;
			return true; // New discovery
		}
		else {
			// Update knowledge if improved
			if (knowledge > it->second.bestKnowledge) {
				it->second.bestKnowledge = knowledge;
			}
			return false; // Already known
		}
	}

	// Record a kill for an existing entry
	void RecordKill(const std::string& name) {
		auto it = entries_.find(name);
		if (it != entries_.end()) {
			it->second.timesDefeated++;
		}
	}

	// Record that the player discovered an enemy's weakness by exploiting it
	// Returns true if this is a NEW weakness discovery
	bool RecordWeaknessDiscovered(const std::string& name) {
		auto it = entries_.find(name);
		if (it != entries_.end() && !it->second.weaknessDiscovered) {
			it->second.weaknessDiscovered = true;
			return true;
		}
		return false;
	}

	// Check if the weakness for an enemy has been discovered
	bool IsWeaknessKnown(const std::string& name) const {
		auto it = entries_.find(name);
		if (it != entries_.end()) {
			return it->second.weaknessDiscovered
				|| it->second.bestKnowledge == EnemyKnowledge::Full;
		}
		return false;
	}

	// Get the weakness element for a known enemy (only if discovered)
	SpellElement GetWeakness(const std::string& name) const {
		auto it = entries_.find(name);
		if (it != entries_.end()) return it->second.weakness;
		return SpellElement::Arcane;
	}

	// Check if an enemy type has been seen before
	bool HasEntry(const std::string& name) const {
		return entries_.find(name) != entries_.end();
	}

	// Get the best knowledge level for a known enemy
	EnemyKnowledge GetKnowledge(const std::string& name) const {
		auto it = entries_.find(name);
		if (it != entries_.end()) return it->second.bestKnowledge;
		return EnemyKnowledge::None;
	}

	int GetEntryCount() const { return static_cast<int>(entries_.size()); }

	// Display the full bestiary (one entry per page)
	void Print() const {
		Console::Clear();
		std::cout << "+==========================================+\n";
		std::cout << "|              B E S T I A R Y             |\n";
		std::cout << "+==========================================+\n\n";

		if (entries_.empty()) {
			std::cout << "  No creatures discovered yet.\n\n";
			Console::WaitForEnter();
			Console::Clear();
			return;
		}

		int index = 1;
		for (auto it = entries_.begin(); it != entries_.end(); ++it, ++index) {
			const BestiaryEntry& e = it->second;

			std::cout << "  [" << index << "/" << entries_.size() << "] " << e.name
				<< "  (defeated " << e.timesDefeated << "x)\n";

			// ASCII art
			const std::string& art = AsciiArt::GetEnemyArt(e.name);
			if (!art.empty()) {
				std::cout << art << "\n";
			}

			// Flavor description
			if (!e.description.empty()) {
				std::cout << "  Note (INT " << e.discoveredByIntelligence << "): "
					<< e.description << "\n\n";
			}

			switch (e.bestKnowledge) {
			case EnemyKnowledge::None:
				std::cout << "  Stats: ???\n";
				break;
			case EnemyKnowledge::Approximate:
				std::cout << "  Stats: ~" << e.maxHp << " HP | ~" << e.atk
					<< " ATK | ~" << e.speed << " SPD\n";
				break;
			case EnemyKnowledge::Partial:
				std::cout << "  Stats: " << e.maxHp << " HP | " << e.atk
					<< " ATK | " << e.speed << " SPD\n";
				break;
			case EnemyKnowledge::Full: {
				std::cout << "  Stats: " << e.maxHp << " HP | " << e.atk
					<< " ATK | " << e.speed << " SPD | "
					<< e.intelligence << " INT\n";
				Spell tmp;
				tmp.element = e.weakness;
				std::cout << "  Weakness: " << tmp.GetElementName() << "\n";
				if (!e.spellNames.empty()) {
					std::cout << "  Spells: ";
					for (size_t i = 0; i < e.spellNames.size(); ++i) {
						if (i > 0) std::cout << ", ";
						std::cout << e.spellNames[i];
					}
					std::cout << "\n";
				}
				break;
			}
			}

			// Show weakness if discovered (but not already shown via Full knowledge)
			if (e.weaknessDiscovered && e.bestKnowledge != EnemyKnowledge::Full) {
				Spell tmp;
				tmp.element = e.weakness;
				std::cout << "  Weakness: " << tmp.GetElementName() << " (discovered!)\n";
			}

			std::cout << "\n";

			if (std::next(it) != entries_.end()) {
				Console::WaitForEnter("  Press Enter for next entry...");
				Console::Clear();
				std::cout << "+==========================================+\n";
				std::cout << "|              B E S T I A R Y             |\n";
				std::cout << "+==========================================+\n\n";
			}
			else {
				Console::WaitForEnter("  Press Enter to close the bestiary...");
				Console::Clear();
			}
		}
	}

private:
	std::map<std::string, BestiaryEntry> entries_;
};
