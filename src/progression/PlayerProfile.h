#pragma once

#include "core/Bestiary.h"
#include "core/Relic.h"

#include <set>
#include <string>
#include <vector>

// Persistent progression is deliberately separate from the run-scoped Player.
// A profile records Legacy, unlocks, settings and Bestiary knowledge across deaths.
struct CompletedRun {
	int floorsCleared = 0;
	int highestFloorReached = 1;
	int enemiesDefeated = 0;
	bool escaped = false;
};

struct LegacyReward {
	int xpEarned = 0;
	int previousRank = 0;
	int newRank = 0;
	std::vector<RelicId> newlyUnlockedRelics;
};

class PlayerProfile {
public:
	static constexpr int CurrentVersion = 2;
	static constexpr int MaximumLegacyRank = 50;

	PlayerProfile();

	int GetLegacyXP() const;
	int GetLegacyRank() const;
	int GetXPIntoRank() const;
	int GetXPForNextRank() const;
	int GetRunCount() const;
	int GetDeathCount() const;
	int GetEscapeCount() const;
	int GetHighestFloor() const;
	int GetTotalKills() const;

	bool IsRelicUnlocked(RelicId id) const;
	const std::set<RelicId>& GetUnlockedRelics() const;
	Bestiary& GetBestiary();
	const Bestiary& GetBestiary() const;

	int GetMusicVolume() const;
	bool IsMusicMuted() const;
	void SetMusicVolume(int percent);
	void SetMusicMuted(bool muted);

	LegacyReward CompleteRun(const CompletedRun& run);

	std::string Serialize() const;
	static bool Deserialize(const std::string& text, PlayerProfile& profile,
		std::string* errorMessage = nullptr);

	static int XPRequiredForRank(int rank);
	static int CalculateRunXP(const CompletedRun& run);

private:
	int version_ = CurrentVersion;
	int legacyXP_ = 0;
	int runs_ = 0;
	int deaths_ = 0;
	int escapes_ = 0;
	int highestFloor_ = 0;
	int totalKills_ = 0;
	int musicVolume_ = 70;
	bool musicMuted_ = false;
	std::set<RelicId> unlockedRelics_;
	Bestiary bestiary_;

	void RefreshRankUnlocks(std::vector<RelicId>* newlyUnlocked = nullptr);
};
