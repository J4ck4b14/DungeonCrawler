#pragma once

// Simple development mode toggles and tunables.
// Enabled automatically when the hero's name is "Dev11032001".
//
// Exposed as a minimal namespace so other systems can query behavior.

namespace DevMode {
	void Enable();
	bool IsEnabled();

	// Enemy stat / XP scaling (1.0 = normal)
	float GetEnemyScale();
	void SetEnemyScale(float s);

	// Trap / chest frequency multiplier
	float GetTrapMultiplier();
	void SetTrapMultiplier(float v);

	// Perception penalty (subtract from raw d20 roll; integer >= 0)
	int GetPerceptionPenalty();
	void SetPerceptionPenalty(int p);

	// Reveal entire map (useful for visual debugging)
	bool RevealMapEnabled();
	void SetRevealMapEnabled(bool v);

	// Remove starting items (for harder testing)
	bool RemoveStartingItems();
	void SetRemoveStartingItems(bool v);

	// Reset tunables back to sane defaults
	void ResetToDefaults();
}