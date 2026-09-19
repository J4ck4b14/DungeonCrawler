#pragma once

#include "entities/Enemy.h"

#include <map>
#include <set>
#include <string>
#include <vector>

// Persistent, species-level field knowledge. Specimen statistics are retained
// as maxima observed, while tactical knowledge only ever improves.
struct BestiaryEntry {
	std::string name;
	EnemyArchetype archetype = EnemyArchetype::Slime;
	bool encountered = false;
	int encounterCount = 0;
	int defeatedCount = 0;
	EnemyKnowledge bestKnowledge = EnemyKnowledge::None;
	bool weaknessDiscovered = false;
	SpellElement weakness = SpellElement::Arcane;
	std::set<std::string> knownSpells;
	std::set<StatusType> knownStatusThreats;
	std::set<std::string> behaviorDiscoveries;
	int patternUnderstanding = 0;
	int highestEnemyRankObserved = 0;
	int maximumHpObserved = 0;
	int maximumStrengthObserved = 0;
	int maximumSpeedObserved = 0;
	int maximumIntelligenceObserved = 0;
	int bestObserverIntelligence = 0;
};

class Bestiary {
public:
	bool RecordEncounter(const Enemy& enemy, EnemyKnowledge knowledge,
		int observerIntelligence = 0);
	bool ImproveKnowledge(const Enemy& enemy, EnemyKnowledge knowledge,
		int observerIntelligence = 0);
	void RecordKill(const std::string& name);
	bool RecordWeaknessDiscovered(const std::string& name);
	void RecordSpellObserved(const std::string& name, const std::string& spellName);
	void RecordStatusObserved(const std::string& name, StatusType status);
	void RecordBehaviorObserved(const std::string& name, const std::string& discovery);

	bool IsWeaknessKnown(const std::string& name) const;
	SpellElement GetWeakness(const std::string& name) const;
	bool HasEntry(const std::string& name) const;
	EnemyKnowledge GetKnowledge(const std::string& name) const;
	const BestiaryEntry* GetEntry(const std::string& name) const;
	int GetEntryCount() const;

	std::vector<std::string> SerializeEntries() const;
	bool DeserializeEntry(const std::string& serialized,
		std::string* errorMessage = nullptr);
	void Print() const;

private:
	std::map<std::string, BestiaryEntry> entries_;

	static void RefreshDiscoveries(BestiaryEntry& entry, const Enemy* enemy);
};
