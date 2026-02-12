#include "core/DevMode.h"

namespace DevMode {

	static bool s_enabled = false;
	static float s_enemyScale = 1.0f;
	static float s_trapMultiplier = 1.0f;
	static int   s_perceptionPenalty = 0;
	static bool  s_revealMap = false;
	static bool  s_removeStartingItems = false;

	void Enable() { s_enabled = true; }
	bool IsEnabled() { return s_enabled; }

	float GetEnemyScale() { return s_enemyScale; }
	void SetEnemyScale(float s) { s_enemyScale = (s > 0.1f) ? s : 0.1f; }

	float GetTrapMultiplier() { return s_trapMultiplier; }
	void SetTrapMultiplier(float v) { s_trapMultiplier = (v > 0.0f) ? v : 0.0f; }

	int GetPerceptionPenalty() { return s_perceptionPenalty; }
	void SetPerceptionPenalty(int p) { s_perceptionPenalty = (p >= 0) ? p : 0; }

	bool RevealMapEnabled() { return s_revealMap; }
	void SetRevealMapEnabled(bool v) { s_revealMap = v; }

	bool RemoveStartingItems() { return s_removeStartingItems; }
	void SetRemoveStartingItems(bool v) { s_removeStartingItems = v; }

	void ResetToDefaults() {
		s_enemyScale = 1.0f;
		s_trapMultiplier = 1.0f;
		s_perceptionPenalty = 0;
		s_revealMap = false;
		s_removeStartingItems = false;
	}
}