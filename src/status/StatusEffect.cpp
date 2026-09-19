#include "StatusEffect.h"

#include <algorithm>
#include <cstddef>

namespace {

size_t StatusIndex(StatusType type) {
	return static_cast<size_t>(type);
}

void ExpireIfFinished(StatusEffect& effect) {
	if (effect.potency <= 0 || effect.remainingTurns <= 0) {
		effect.potency = 0;
		effect.remainingTurns = 0;
	}
}

} // namespace

bool StatusEffect::IsActive() const {
	return potency > 0
		&& (type == StatusType::Bleed || remainingTurns > 0);
}

int StatusTurnResult::TotalDamage() const {
	return poisonDamage + burnDamage + bleedDamage;
}

StatusEffect& StatusContainer::GetMutable(StatusType type) {
	return effects_[StatusIndex(type)];
}

const StatusEffect& StatusContainer::Get(StatusType type) const {
	return effects_[StatusIndex(type)];
}

bool StatusContainer::Has(StatusType type) const {
	return Get(type).IsActive();
}

void StatusContainer::ApplyPoison(int amount) {
	if (amount <= 0) return;
	StatusEffect& poison = GetMutable(StatusType::Poison);
	poison.potency += amount;
	poison.remainingTurns = poison.potency;
}

void StatusContainer::ApplyBurn(int stacks, int durationTurns) {
	if (stacks <= 0 || durationTurns <= 0) return;
	StatusEffect& burn = GetMutable(StatusType::Burn);
	burn.potency += stacks;
	burn.remainingTurns = std::max(burn.remainingTurns, durationTurns);
}

void StatusContainer::ApplyFreeze(int actions) {
	if (actions <= 0) return;
	StatusEffect& freeze = GetMutable(StatusType::Freeze);
	freeze.potency += actions;
	freeze.remainingTurns = freeze.potency;
}

void StatusContainer::ApplyBleed(int charge) {
	if (charge <= 0) return;
	StatusEffect& bleed = GetMutable(StatusType::Bleed);
	bleed.potency += charge;
	// One status-processing interval may pass before a quiet turn drains charge.
	bleed.remainingTurns = 1;
}

void StatusContainer::ApplyRegeneration(int stacks, int durationTurns) {
	if (stacks <= 0 || durationTurns <= 0) return;
	StatusEffect& regeneration = GetMutable(StatusType::Regeneration);
	regeneration.potency += stacks;
	regeneration.remainingTurns = std::max(regeneration.remainingTurns, durationTurns);
}

void StatusContainer::Clear(StatusType type) {
	StatusEffect& effect = GetMutable(type);
	effect.potency = 0;
	effect.remainingTurns = 0;
}

void StatusContainer::ClearAll() {
	for (StatusEffect& effect : effects_) {
		effect.potency = 0;
		effect.remainingTurns = 0;
	}
}

StatusTurnResult StatusContainer::ProcessTurnStart(bool regenerationSuppressed) {
	StatusTurnResult result;

	StatusEffect& poison = GetMutable(StatusType::Poison);
	if (poison.IsActive()) {
		result.poisonDamage = poison.potency;
		--poison.potency;
		poison.remainingTurns = poison.potency;
		ExpireIfFinished(poison);
	}

	StatusEffect& burn = GetMutable(StatusType::Burn);
	if (burn.IsActive()) {
		result.burnDamage = burn.potency * StatusTuning::BurnDamagePerStack;
		--burn.remainingTurns;
		ExpireIfFinished(burn);
	}

	StatusEffect& freeze = GetMutable(StatusType::Freeze);
	if (freeze.IsActive()) {
		result.skipAction = true;
		--freeze.potency;
		freeze.remainingTurns = freeze.potency;
		ExpireIfFinished(freeze);
	}

	StatusEffect& bleed = GetMutable(StatusType::Bleed);
	if (bleed.IsActive()) {
		if (bleed.potency >= StatusTuning::BleedBurstThreshold) {
			result.bleedDamage = bleed.potency
				* StatusTuning::BleedBurstDamagePerCharge;
			bleed.potency = 0;
			bleed.remainingTurns = 0;
		}
		else if (bleed.remainingTurns > 0) {
			--bleed.remainingTurns;
		}
		else {
			bleed.potency = std::max(0,
				bleed.potency - StatusTuning::BleedDecayPerQuietTurn);
		}
	}

	StatusEffect& regeneration = GetMutable(StatusType::Regeneration);
	if (regeneration.IsActive()) {
		if (!regenerationSuppressed) {
			result.regenerationHealing = regeneration.potency
				* StatusTuning::RegenerationHealingPerStack;
		}
		--regeneration.remainingTurns;
		ExpireIfFinished(regeneration);
	}

	return result;
}
