#include "Bestiary.h"

#include "combat/SpellRules.h"
#include "core/EnemyDescriptions.h"
#include "entities/EnemyBehavior.h"
#include "utils/AsciiArt.h"
#include "utils/Console.h"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {

bool ParseInt(std::string_view text, int& value) {
	if (text.empty()) return false;
	const char* begin = text.data();
	const char* end = begin + text.size();
	const auto result = std::from_chars(begin, end, value);
	return result.ec == std::errc{} && result.ptr == end;
}

std::vector<std::string> SplitKeepingEmpty(const std::string& value, char delimiter) {
	std::vector<std::string> parts;
	std::string part;
	std::istringstream stream(value);
	while (std::getline(stream, part, delimiter)) parts.push_back(part);
	if (!value.empty() && value.back() == delimiter) parts.emplace_back();
	return parts;
}

std::set<std::string> ParseStrings(const std::string& value) {
	std::set<std::string> result;
	for (const std::string& part : SplitKeepingEmpty(value, ',')) {
		if (!part.empty()) result.insert(part);
	}
	return result;
}

std::string JoinStrings(const std::set<std::string>& values) {
	std::string result;
	for (const std::string& value : values) {
		if (!result.empty()) result += ',';
		result += value;
	}
	return result;
}

const char* StatusName(StatusType status) {
	switch (status) {
	case StatusType::Burn: return "Burn";
	case StatusType::Poison: return "Poison";
	case StatusType::Freeze: return "Freeze/disruption";
	case StatusType::Bleed: return "Bleed";
	case StatusType::Regeneration: return "Regeneration";
	case StatusType::Count: break;
	}
	return "Unknown";
}

const char* UnderstandingName(int value) {
	if (value >= 3) return "expert";
	if (value == 2) return "practiced";
	if (value == 1) return "familiar";
	return "impression only";
}

} // namespace

void Bestiary::RefreshDiscoveries(BestiaryEntry& entry, const Enemy* enemy) {
	const int knowledge = static_cast<int>(entry.bestKnowledge);
	entry.patternUnderstanding = std::max(entry.patternUnderstanding, knowledge);
	if (knowledge >= 1) entry.behaviorDiscoveries.insert("temperament");
	if (knowledge >= 2) {
		entry.behaviorDiscoveries.insert("cadence");
		entry.behaviorDiscoveries.insert("commitments");
	}
	if (knowledge >= 3) {
		entry.behaviorDiscoveries.insert("rank_pressure");
		entry.behaviorDiscoveries.insert("counterplay");
	}
	// Knowledge improves interpretation, but concrete spells and status threats
	// are only stored by their corresponding witnessed combat events.
	(void)enemy;
}

bool Bestiary::RecordEncounter(const Enemy& enemy, EnemyKnowledge knowledge,
	int observerIntelligence) {
	const bool isNew = !entries_.contains(enemy.GetName());
	BestiaryEntry& entry = entries_[enemy.GetName()];
	entry.name = enemy.GetName();
	entry.archetype = enemy.GetArchetype();
	entry.encountered = true;
	++entry.encounterCount;
	entry.bestObserverIntelligence = std::max(entry.bestObserverIntelligence,
		std::max(0, observerIntelligence));
	entry.highestEnemyRankObserved = std::max(entry.highestEnemyRankObserved,
		enemy.GetRank());
	entry.maximumHpObserved = std::max(entry.maximumHpObserved, enemy.GetMaxHP());
	entry.maximumStrengthObserved = std::max(entry.maximumStrengthObserved,
		enemy.GetStrength());
	entry.maximumSpeedObserved = std::max(entry.maximumSpeedObserved, enemy.GetSpeed());
	entry.maximumIntelligenceObserved = std::max(entry.maximumIntelligenceObserved,
		enemy.GetIntelligence());
	entry.weakness = enemy.GetWeakness();
	entry.bestKnowledge = std::max(entry.bestKnowledge, knowledge);
	if (entry.encounterCount >= 2)
		entry.bestKnowledge = std::max(entry.bestKnowledge, EnemyKnowledge::Approximate);
	if (entry.encounterCount >= 6)
		entry.bestKnowledge = std::max(entry.bestKnowledge, EnemyKnowledge::Partial);
	RefreshDiscoveries(entry, &enemy);
	return isNew;
}

bool Bestiary::ImproveKnowledge(const Enemy& enemy, EnemyKnowledge knowledge,
	int observerIntelligence) {
	if (!entries_.contains(enemy.GetName()))
		return RecordEncounter(enemy, knowledge, observerIntelligence);
	BestiaryEntry& entry = entries_[enemy.GetName()];
	const EnemyKnowledge before = entry.bestKnowledge;
	entry.bestKnowledge = std::max(entry.bestKnowledge, knowledge);
	entry.bestObserverIntelligence = std::max(entry.bestObserverIntelligence,
		std::max(0, observerIntelligence));
	entry.highestEnemyRankObserved = std::max(entry.highestEnemyRankObserved,
		enemy.GetRank());
	RefreshDiscoveries(entry, &enemy);
	return entry.bestKnowledge > before;
}

void Bestiary::RecordKill(const std::string& name) {
	if (auto found = entries_.find(name); found != entries_.end())
		++found->second.defeatedCount;
}

bool Bestiary::RecordWeaknessDiscovered(const std::string& name) {
	auto found = entries_.find(name);
	if (found == entries_.end() || found->second.weaknessDiscovered) return false;
	found->second.weaknessDiscovered = true;
	return true;
}

void Bestiary::RecordSpellObserved(const std::string& name,
	const std::string& spellName) {
	if (auto found = entries_.find(name); found != entries_.end())
		found->second.knownSpells.insert(spellName);
}

void Bestiary::RecordStatusObserved(const std::string& name, StatusType status) {
	if (auto found = entries_.find(name); found != entries_.end())
		found->second.knownStatusThreats.insert(status);
}

void Bestiary::RecordBehaviorObserved(const std::string& name,
	const std::string& discovery) {
	if (auto found = entries_.find(name); found != entries_.end() && !discovery.empty())
		found->second.behaviorDiscoveries.insert(discovery);
}

bool Bestiary::IsWeaknessKnown(const std::string& name) const {
	const BestiaryEntry* entry = GetEntry(name);
	return entry && (entry->weaknessDiscovered
		|| entry->bestKnowledge == EnemyKnowledge::Full);
}

SpellElement Bestiary::GetWeakness(const std::string& name) const {
	const BestiaryEntry* entry = GetEntry(name);
	return entry ? entry->weakness : SpellElement::Arcane;
}

bool Bestiary::HasEntry(const std::string& name) const { return entries_.contains(name); }

EnemyKnowledge Bestiary::GetKnowledge(const std::string& name) const {
	const BestiaryEntry* entry = GetEntry(name);
	return entry ? entry->bestKnowledge : EnemyKnowledge::None;
}

const BestiaryEntry* Bestiary::GetEntry(const std::string& name) const {
	const auto found = entries_.find(name);
	return found == entries_.end() ? nullptr : &found->second;
}

int Bestiary::GetEntryCount() const { return static_cast<int>(entries_.size()); }

std::vector<std::string> Bestiary::SerializeEntries() const {
	std::vector<std::string> lines;
	for (const auto& [name, entry] : entries_) {
		std::ostringstream output;
		output << name << '|' << static_cast<int>(entry.archetype)
			<< '|' << entry.encounterCount << '|' << entry.defeatedCount
			<< '|' << static_cast<int>(entry.bestKnowledge)
			<< '|' << (entry.weaknessDiscovered ? 1 : 0)
			<< '|' << static_cast<int>(entry.weakness)
			<< '|' << entry.highestEnemyRankObserved
			<< '|' << entry.maximumHpObserved
			<< '|' << entry.maximumStrengthObserved
			<< '|' << entry.maximumSpeedObserved
			<< '|' << entry.maximumIntelligenceObserved
			<< '|' << entry.bestObserverIntelligence
			<< '|' << entry.patternUnderstanding
			<< '|' << JoinStrings(entry.knownSpells) << '|';
		bool first = true;
		for (StatusType status : entry.knownStatusThreats) {
			if (!first) output << ',';
			first = false;
			output << static_cast<int>(status);
		}
		output << '|' << JoinStrings(entry.behaviorDiscoveries);
		lines.push_back(output.str());
	}
	return lines;
}

bool Bestiary::DeserializeEntry(const std::string& serialized,
	std::string* errorMessage) {
	const std::vector<std::string> fields = SplitKeepingEmpty(serialized, '|');
	if (fields.size() != 17 || fields[0].empty()) {
		if (errorMessage) *errorMessage = "Malformed Bestiary entry.";
		return false;
	}
	int values[13]{};
	for (int i = 0; i < 13; ++i) {
		if (!ParseInt(fields[static_cast<std::size_t>(i + 1)], values[i])) {
			if (errorMessage) *errorMessage = "Invalid number in Bestiary entry.";
			return false;
		}
	}
	if (values[0] < 0 || values[0] >= static_cast<int>(EnemyArchetype::Count)
		|| values[3] < 0 || values[3] > static_cast<int>(EnemyKnowledge::Full)
		|| values[5] < 0 || values[5] > static_cast<int>(SpellElement::Arcane)) {
		if (errorMessage) *errorMessage = "Out-of-range Bestiary value.";
		return false;
	}
	BestiaryEntry entry;
	entry.name = fields[0];
	entry.archetype = static_cast<EnemyArchetype>(values[0]);
	entry.encountered = true;
	entry.encounterCount = std::max(1, values[1]);
	entry.defeatedCount = std::max(0, values[2]);
	entry.bestKnowledge = static_cast<EnemyKnowledge>(values[3]);
	entry.weaknessDiscovered = values[4] != 0;
	entry.weakness = static_cast<SpellElement>(values[5]);
	entry.highestEnemyRankObserved = std::clamp(values[6], 1, 50);
	entry.maximumHpObserved = std::max(0, values[7]);
	entry.maximumStrengthObserved = std::max(0, values[8]);
	entry.maximumSpeedObserved = std::max(0, values[9]);
	entry.maximumIntelligenceObserved = std::max(0, values[10]);
	entry.bestObserverIntelligence = std::max(0, values[11]);
	entry.patternUnderstanding = std::clamp(values[12], 0, 3);
	entry.knownSpells = ParseStrings(fields[14]);
	for (const std::string& statusText : SplitKeepingEmpty(fields[15], ',')) {
		if (statusText.empty()) continue;
		int status = 0;
		if (!ParseInt(statusText, status)
			|| status < 0 || status >= static_cast<int>(StatusType::Count)) {
			if (errorMessage) *errorMessage = "Invalid Bestiary status.";
			return false;
		}
		entry.knownStatusThreats.insert(static_cast<StatusType>(status));
	}
	entry.behaviorDiscoveries = ParseStrings(fields[16]);
	RefreshDiscoveries(entry, nullptr);
	entries_[entry.name] = std::move(entry);
	return true;
}

void Bestiary::Print() const {
	Console::Clear();
	std::cout << "+==========================================+\n"
		<< "|              B E S T I A R Y             |\n"
		<< "+==========================================+\n\n";
	if (entries_.empty()) {
		std::cout << "  No creatures documented yet.\n\n";
		Console::WaitForEnter();
		Console::Clear();
		return;
	}

	int index = 1;
	for (auto iterator = entries_.begin(); iterator != entries_.end(); ++iterator, ++index) {
		const BestiaryEntry& entry = iterator->second;
		std::cout << "  [" << index << "/" << entries_.size() << "] " << entry.name
			<< "  (encountered " << entry.encounterCount << "x, defeated "
			<< entry.defeatedCount << "x)\n";
		const std::string& art = AsciiArt::GetEnemyArt(entry.name);
		if (!art.empty()) std::cout << art << '\n';
		std::cout << "  Field notes: " << EnemyDescriptions::GetDescription(
			entry.name, static_cast<int>(entry.bestKnowledge)) << "\n";
		std::cout << "  Pattern understanding: "
			<< UnderstandingName(entry.patternUnderstanding) << "\n";
		std::cout << "  Highest Rank observed: " << entry.highestEnemyRankObserved << "\n";
		if (entry.bestKnowledge >= EnemyKnowledge::Partial) {
			std::cout << "  Observed maxima: HP " << entry.maximumHpObserved
				<< " | STR " << entry.maximumStrengthObserved
				<< " | SPD " << entry.maximumSpeedObserved
				<< " | INT " << entry.maximumIntelligenceObserved << "\n";
		}
		if (IsWeaknessKnown(entry.name)) {
			Spell element;
			element.element = entry.weakness;
			std::cout << "  Practical weakness: " << element.GetElementName() << "\n";
		}
		if (!entry.knownSpells.empty()) {
			std::cout << "  Observed spells: ";
			bool first = true;
			for (const std::string& spell : entry.knownSpells) {
				if (!first) std::cout << ", ";
				first = false;
				std::cout << spell;
			}
			std::cout << '\n';
		}
		if (!entry.knownStatusThreats.empty()) {
			std::cout << "  Status threats: ";
			bool first = true;
			for (StatusType status : entry.knownStatusThreats) {
				if (!first) std::cout << ", ";
				first = false;
				std::cout << StatusName(status);
			}
			std::cout << '\n';
		}
		std::cout << '\n';
		if (std::next(iterator) != entries_.end()) {
			Console::WaitForEnter("  Press Enter for next entry...");
			Console::Clear();
		}
		else Console::WaitForEnter("  Press Enter to close the bestiary...");
	}
	Console::Clear();
}
