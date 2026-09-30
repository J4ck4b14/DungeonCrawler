#pragma once

// Optional runtime tuning used while testing balance and dungeon generation.
// Native builds enable it explicitly with the --dev command-line flag; normal
// runs never infer developer state from player data such as the hero's name.

namespace DevMode {
	void Enable();
	void Disable();
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
