#pragma once

#include <array>
#include <cstddef>

enum class StatusType {
	Burn,
	Poison,
	Freeze,
	Bleed,
	Regeneration,
	Count
};

namespace StatusTuning {

inline constexpr int BurnDamagePerStack = 2;
inline constexpr int BurnDurationTurns = 3;
inline constexpr int FreezeActionsPerApplication = 1;
inline constexpr int BleedBurstThreshold = 10;
inline constexpr int BleedBurstDamagePerCharge = 1;
inline constexpr int BleedDecayPerQuietTurn = 2;
inline constexpr int RegenerationHealingPerStack = 2;
inline constexpr int RegenerationDurationTurns = 3;

} // namespace StatusTuning

struct StatusEffect {
	StatusType type = StatusType::Burn;
	int potency = 0;
	int remainingTurns = 0;

	bool IsActive() const;
};

struct StatusTurnResult {
	int poisonDamage = 0;
	int burnDamage = 0;
	int bleedDamage = 0;
	int regenerationHealing = 0;
	bool skipAction = false;

	int TotalDamage() const;
};

class StatusContainer {
public:
	void ApplyPoison(int amount);
	void ApplyBurn(int stacks = 1,
		int durationTurns = StatusTuning::BurnDurationTurns);
	void ApplyFreeze(int actions = StatusTuning::FreezeActionsPerApplication);
	void ApplyBleed(int charge = 1);
	void ApplyRegeneration(int stacks = 1,
		int durationTurns = StatusTuning::RegenerationDurationTurns);

	const StatusEffect& Get(StatusType type) const;
	bool Has(StatusType type) const;
	void Clear(StatusType type);
	void ClearAll();

	StatusTurnResult ProcessTurnStart(bool regenerationSuppressed = false);

private:
	std::array<StatusEffect, static_cast<std::size_t>(StatusType::Count)> effects_ = {{
		{StatusType::Burn, 0, 0},
		{StatusType::Poison, 0, 0},
		{StatusType::Freeze, 0, 0},
		{StatusType::Bleed, 0, 0},
		{StatusType::Regeneration, 0, 0}
	}};

	StatusEffect& GetMutable(StatusType type);
};
