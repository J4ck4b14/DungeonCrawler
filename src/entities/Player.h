// Player.h
// --------
// The player entity. Extends Entity with:
//   - Inventory for consumable items
//   - XP / leveling system (3 stat points per level)
//   - Character creation via AllocateStats() (distributes 5 starting points)
//   - Equipment-derived combat bonuses and uncapped rest-site training

#pragma once
#include "Entity.h"
#include "equipment/Equipment.h"
#include "items/Inventory.h"
#include "core/Relic.h"

class Player : public Entity {
public:
	Player(const std::string& name, const Stats& stats);

	Inventory& GetInventory();
	const Inventory& GetInventory() const;
	const EquipmentSlots& GetEquipment() const;
	bool EquipWeapon(Weapon weapon);
	bool EquipApparel(Apparel apparel);

	// Stat allocation at character creation
	static Stats AllocateStats(int pool);

	// Level-up: spend 3 points
	void AllocateLevelUpPoints();

	// Try to learn a spell from a defeated enemy
	bool TryLearnSpell(const Spell& spell);

	// XP system
	void GainXP(int amount);
	int GetXP() const;
	int GetLevel() const;
	int GetXPToNextLevel() const;

	void PrintStatus() const;

	// Access raw point allocations for level-up recalculation
	int GetRawHP() const;

	// Training system (rest areas): one STR, SPD, or INT point per site.
	int GetTrainingPoints() const;
	bool CanTrain() const;
	bool TrainStat(int statChoice); // 1=STR, 2=SPD, 3=INT

	int GetSpeed() const override;
	int GetMaxMana() const override;
	int GetWeaponDamage() const;
	int GetArmor() const;
	int GetSpellPowerBonus() const;
	bool SharpenWeapon();
	bool ImproveApparel(ApparelSlot slot);

	// Death save counter: each use makes the next QTE harder
	int GetDeathSaveCount() const;
	void IncrementDeathSave();

	// Relic system (roguelike boons chosen after each floor)
	bool HasRelic(RelicId id) const;
	void GrantRelic(RelicId id);
	const std::vector<RelicId>& GetRelics() const;
	int GetEffectiveManaCost(const Spell& spell) const;

private:
	Inventory inventory_;
	EquipmentSlots equipment_;
	int xp_ = 0;
	int level_ = 1;
	int rawHp_ = 0;
	int trainingPoints_ = 0;
	int deathSaveCount_ = 0;      // How many times the player has cheated death
	std::vector<RelicId> relics_; // Relics collected this run
	int relicMaxHpMod_ = 0;       // Net max-HP change from relics (survives level-ups)

	// Re-derive maxHp/maxMana and re-apply relic HP modifiers
	void RecalcDerivedWithRelics();

	void CheckLevelUp();
	static int XPForLevel(int level);
};
