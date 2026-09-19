#include "PlayerProfile.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace {

bool ParseInt(std::string_view text, int& value) {
	if (text.empty()) return false;
	const char* begin = text.data();
	const char* end = begin + text.size();
	const auto result = std::from_chars(begin, end, value);
	return result.ec == std::errc{} && result.ptr == end;
}

std::vector<std::string> Split(const std::string& value, char delimiter) {
	std::vector<std::string> parts;
	std::istringstream stream(value);
	std::string part;
	while (std::getline(stream, part, delimiter)) {
		if (!part.empty()) parts.push_back(part);
	}
	return parts;
}

int OldXPRequiredForRank(int rank) {
	if (rank <= 1) return 0;
	return 25 * (rank - 1) * (rank + 1);
}

int MigrateVersionOneXP(int oldXp) {
	oldXp = std::max(0, oldXp);
	int oldRank = 1;
	while (oldRank < 100 && oldXp >= OldXPRequiredForRank(oldRank + 1)) ++oldRank;
	const int mappedRank = std::min(oldRank, PlayerProfile::MaximumLegacyRank);
	if (mappedRank >= PlayerProfile::MaximumLegacyRank)
		return PlayerProfile::XPRequiredForRank(PlayerProfile::MaximumLegacyRank);
	const int oldFloor = OldXPRequiredForRank(oldRank);
	const int oldSpan = std::max(1, OldXPRequiredForRank(oldRank + 1) - oldFloor);
	const double progress = static_cast<double>(oldXp - oldFloor) / oldSpan;
	const int newFloor = PlayerProfile::XPRequiredForRank(mappedRank);
	const int newSpan = PlayerProfile::XPRequiredForRank(mappedRank + 1) - newFloor;
	return newFloor + static_cast<int>(std::floor(progress * newSpan));
}

} // namespace

PlayerProfile::PlayerProfile() {
	RefreshRankUnlocks();
}

int PlayerProfile::GetLegacyXP() const { return legacyXP_; }

int PlayerProfile::GetLegacyRank() const {
	int rank = 0;
	while (rank < MaximumLegacyRank
		&& legacyXP_ >= XPRequiredForRank(rank + 1)) ++rank;
	return rank;
}

int PlayerProfile::GetXPIntoRank() const {
	if (GetLegacyRank() >= MaximumLegacyRank) return 0;
	return legacyXP_ - XPRequiredForRank(GetLegacyRank());
}

int PlayerProfile::GetXPForNextRank() const {
	const int rank = GetLegacyRank();
	if (rank >= MaximumLegacyRank) return 0;
	return XPRequiredForRank(rank + 1) - XPRequiredForRank(rank);
}

int PlayerProfile::GetRunCount() const { return runs_; }
int PlayerProfile::GetDeathCount() const { return deaths_; }
int PlayerProfile::GetEscapeCount() const { return escapes_; }
int PlayerProfile::GetHighestFloor() const { return highestFloor_; }
int PlayerProfile::GetTotalKills() const { return totalKills_; }

bool PlayerProfile::IsRelicUnlocked(RelicId id) const {
	return unlockedRelics_.contains(id);
}

const std::set<RelicId>& PlayerProfile::GetUnlockedRelics() const {
	return unlockedRelics_;
}

Bestiary& PlayerProfile::GetBestiary() { return bestiary_; }
const Bestiary& PlayerProfile::GetBestiary() const { return bestiary_; }

int PlayerProfile::GetMusicVolume() const { return musicVolume_; }
bool PlayerProfile::IsMusicMuted() const { return musicMuted_; }

void PlayerProfile::SetMusicVolume(int percent) {
	musicVolume_ = std::clamp(percent, 0, 100);
}

void PlayerProfile::SetMusicMuted(bool muted) {
	musicMuted_ = muted;
}

LegacyReward PlayerProfile::CompleteRun(const CompletedRun& run) {
	LegacyReward reward;
	reward.previousRank = GetLegacyRank();
	reward.xpEarned = CalculateRunXP(run);

	legacyXP_ = std::min(XPRequiredForRank(MaximumLegacyRank),
		legacyXP_ + reward.xpEarned);
	++runs_;
	if (run.escaped) ++escapes_;
	else ++deaths_;
	highestFloor_ = std::max(highestFloor_, std::max(1, run.highestFloorReached));
	totalKills_ += std::max(0, run.enemiesDefeated);
	RefreshRankUnlocks(&reward.newlyUnlockedRelics);
	reward.newRank = GetLegacyRank();
	return reward;
}

int PlayerProfile::XPRequiredForRank(int rank) {
	rank = std::clamp(rank, 0, MaximumLegacyRank);
	return 25 * rank * (rank + 1);
}

int PlayerProfile::CalculateRunXP(const CompletedRun& run) {
	const int floors = std::max(0, run.floorsCleared);
	const int depth = std::max(1, run.highestFloorReached);
	const int kills = std::max(0, run.enemiesDefeated);
	return 10 + floors * 20 + depth * depth * 4 + kills * 2;
}

void PlayerProfile::RefreshRankUnlocks(std::vector<RelicId>* newlyUnlocked) {
	const int rank = GetLegacyRank();
	for (const RelicInfo& info : AllRelics()) {
		if (info.unlockRank > rank) continue;
		const auto [iterator, inserted] = unlockedRelics_.insert(info.id);
		(void)iterator;
		if (inserted && newlyUnlocked) newlyUnlocked->push_back(info.id);
	}
}

std::string PlayerProfile::Serialize() const {
	std::ostringstream output;
	output << "profile_version=" << CurrentVersion << '\n';
	output << "legacy_xp=" << legacyXP_ << '\n';
	output << "runs=" << runs_ << '\n';
	output << "deaths=" << deaths_ << '\n';
	output << "escapes=" << escapes_ << '\n';
	output << "highest_floor=" << highestFloor_ << '\n';
	output << "total_kills=" << totalKills_ << '\n';
	output << "music_volume=" << musicVolume_ << '\n';
	output << "music_muted=" << (musicMuted_ ? 1 : 0) << '\n';
	output << "unlocked_relics=";
	bool first = true;
	for (RelicId id : unlockedRelics_) {
		if (!first) output << ',';
		first = false;
		output << GetRelicInfo(id).key;
	}
	output << '\n';
	for (const std::string& entry : bestiary_.SerializeEntries()) {
		output << "bestiary_entry=" << entry << '\n';
	}
	return output.str();
}

bool PlayerProfile::Deserialize(const std::string& text, PlayerProfile& profile,
	std::string* errorMessage) {
	std::unordered_map<std::string, std::string> fields;
	std::vector<std::string> bestiaryEntries;
	std::istringstream input(text);
	std::string line;
	while (std::getline(input, line)) {
		const size_t separator = line.find('=');
		if (separator == std::string::npos) continue;
		const std::string key = line.substr(0, separator);
		const std::string value = line.substr(separator + 1);
		if (key == "bestiary_entry") bestiaryEntries.push_back(value);
		else fields[key] = value;
	}

	int version = 0;
	if (!fields.contains("profile_version")
		|| !ParseInt(fields["profile_version"], version)
		|| version < 1 || version > CurrentVersion) {
		if (errorMessage) *errorMessage = "Unsupported or missing profile version.";
		return false;
	}

	PlayerProfile parsed;
	parsed.version_ = version;
	auto readNonNegative = [&](const char* key, int& destination) {
		int value = 0;
		if (fields.contains(key) && ParseInt(fields[key], value)) {
			destination = std::max(0, value);
		}
	};
	readNonNegative("legacy_xp", parsed.legacyXP_);
	if (version == 1) parsed.legacyXP_ = MigrateVersionOneXP(parsed.legacyXP_);
	parsed.legacyXP_ = std::min(parsed.legacyXP_, XPRequiredForRank(MaximumLegacyRank));
	readNonNegative("runs", parsed.runs_);
	readNonNegative("deaths", parsed.deaths_);
	readNonNegative("escapes", parsed.escapes_);
	readNonNegative("highest_floor", parsed.highestFloor_);
	readNonNegative("total_kills", parsed.totalKills_);
	readNonNegative("music_volume", parsed.musicVolume_);
	parsed.musicVolume_ = std::clamp(parsed.musicVolume_, 0, 100);
	int muted = 0;
	if (fields.contains("music_muted") && ParseInt(fields["music_muted"], muted)) {
		parsed.musicMuted_ = muted != 0;
	}

	parsed.unlockedRelics_.clear();
	if (fields.contains("unlocked_relics")) {
		for (const std::string& key : Split(fields["unlocked_relics"], ',')) {
			if (const RelicInfo* relic = FindRelicByKey(key)) {
				parsed.unlockedRelics_.insert(relic->id);
			}
		}
	}
	parsed.RefreshRankUnlocks();
	if (version >= 2) {
		for (const std::string& entry : bestiaryEntries) {
			std::string bestiaryError;
			if (!parsed.bestiary_.DeserializeEntry(entry, &bestiaryError)) {
				if (errorMessage) *errorMessage = bestiaryError;
				return false;
			}
		}
	}
	parsed.version_ = CurrentVersion;
	profile = parsed;
	return true;
}
