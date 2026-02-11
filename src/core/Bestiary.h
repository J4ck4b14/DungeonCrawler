// Bestiary.h
// ----------
// Persistent record of all enemy types the player has encountered.
// Each new discovery grants bonus XP. The bestiary stores the best
// knowledge level achieved for each enemy (from Inspect rolls).
// Can be viewed during combat or exploration.

#pragma once
#include "entities/Enemy.h"
#include "combat/Spell.h"
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
};

class Bestiary {
public:
	// Record an enemy encounter. Returns true if this is a NEW discovery.
	bool RecordEnemy(const Enemy& enemy, EnemyKnowledge knowledge) {
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

	// Display the full bestiary
	void Print() const {
		std::cout << "\n+==========================================+\n";
		std::cout << "|              B E S T I A R Y             |\n";
		std::cout << "+==========================================+\n\n";

		if (entries_.empty()) {
			std::cout << "  No creatures discovered yet.\n\n";
			return;
		}

		int index = 1;
		for (const auto& pair : entries_) {
			const BestiaryEntry& e = pair.second;
			std::cout << "  " << index << ". " << e.name;
			std::cout << "  (defeated " << e.timesDefeated << "x)\n";

			switch (e.bestKnowledge) {
			case EnemyKnowledge::None:
				std::cout << "     Stats: ???\n";
				break;
			case EnemyKnowledge::Approximate:
				std::cout << "     Stats: ~" << e.maxHp << " HP | ~"
					<< e.atk << " ATK | ~" << e.speed << " SPD\n";
				break;
			case EnemyKnowledge::Partial:
				std::cout << "     Stats: " << e.maxHp << " HP | "
					<< e.atk << " ATK | " << e.speed << " SPD\n";
				break;
			case EnemyKnowledge::Full: {
				std::cout << "     Stats: " << e.maxHp << " HP | "
					<< e.atk << " ATK | " << e.speed << " SPD | "
					<< e.intelligence << " INT\n";
				Spell tmp;
				tmp.element = e.weakness;
				std::cout << "     Weakness: " << tmp.GetElementName() << "\n";
				if (!e.spellNames.empty()) {
					std::cout << "     Spells: ";
					for (size_t i = 0; i < e.spellNames.size(); ++i) {
						if (i > 0) std::cout << ", ";
						std::cout << e.spellNames[i];
					}
					std::cout << "\n";
				}
				break;
			}
			}
			std::cout << "\n";
			index++;
		}

		std::cout << "+==========================================+\n\n";
	}

private:
	std::map<std::string, BestiaryEntry> entries_;
};
