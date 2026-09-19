#include "DeathSaveRules.h"

#include "utils/RNG.h"

#include <algorithm>

namespace DeathSaveRules {

HeartbeatSettings SettingsFor(int priorSaves, bool phoenixFeather) {
	priorSaves = std::max(0, priorSaves);
	HeartbeatSettings result;
	result.sequenceLength = std::min(12, 4 + priorSaves * 2);
	result.windowMs = std::max(400, 1000 - priorSaves * 100)
		+ (phoenixFeather ? 250 : 0);
	return result;
}

std::vector<int> GenerateSequence(int priorSaves, RNG& rng) {
	const int length = SettingsFor(priorSaves, false).sequenceLength;
	std::vector<int> result;
	result.reserve(length);
	for (int i = 0; i < length; ++i) result.push_back(rng.NextInt(1, 3));
	return result;
}

bool KeyMatches(int expectedDigit, int key) {
	return expectedDigit >= 1 && expectedDigit <= 3
		&& key == '0' + expectedDigit;
}

bool SequenceMatches(const std::vector<int>& sequence,
	const std::vector<int>& keys) {
	if (sequence.size() != keys.size()) return false;
	for (std::size_t i = 0; i < sequence.size(); ++i) {
		if (!KeyMatches(sequence[i], keys[i])) return false;
	}
	return true;
}

}
