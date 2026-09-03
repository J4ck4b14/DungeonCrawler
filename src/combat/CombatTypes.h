#pragma once

enum class ActionType {
	Attack,
	Defend,
	CastSpell,
	UseItem,
	Inspect,
	None
};

enum class AttackStyle {
	Slash,
	Thrust,
	Bash
};

enum class DefenseStance {
	AntiSlash,
	AntiThrust,
	AntiBash,
	AntiMagic
};

struct TurnAction {
	ActionType type = ActionType::None;
	int spellIndex = -1;
	int itemIndex = -1;
	AttackStyle attackStyle = AttackStyle::Slash;
	DefenseStance defenseStance = DefenseStance::AntiSlash;
};
