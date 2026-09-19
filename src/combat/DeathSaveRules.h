#pragma once

#include <vector>

class RNG;

struct HeartbeatSettings {
	int sequenceLength = 4;
	int beatMs = 600;
	int windowMs = 1000;
};

namespace DeathSaveRules {
HeartbeatSettings SettingsFor(int priorSaves, bool phoenixFeather);
std::vector<int> GenerateSequence(int priorSaves, RNG& rng);
bool KeyMatches(int expectedDigit, int key);
bool SequenceMatches(const std::vector<int>& sequence,
	const std::vector<int>& keys);
}
