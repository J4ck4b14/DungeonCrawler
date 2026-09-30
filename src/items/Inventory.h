// Consumable-only inventory. Equipment is intentionally owned by EquipmentSlots,
// so replacing gear and carrying potions remain separate decisions.

#pragma once
#include "Item.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

class Inventory {
public:
	void AddItem(const Item& item) {
		items_.push_back(item);
	}

	bool IsEmpty() const { return items_.empty(); }

	void ListItems() const {
		if (items_.empty()) {
			std::cout << "  (empty)\n";
			return;
		}
		for (size_t i = 0; i < items_.size(); ++i) {
			std::cout << "  " << (i + 1) << ". ";
			items_[i].Describe();
		}
	}

	// A successful use always consumes the selected potion, even at full resource.
	bool UseItem(size_t index, int& hp, int maxHp, int& mana, int maxMana) {
		if (index >= items_.size()) return false;

		const Item& item = items_[index];
		if (item.type == ItemType::HealthPotion) {
			int healed = std::min(item.potency, maxHp - hp);
			hp += healed;
			std::cout << "  Used " << item.name << "! Restored " << healed << " HP. ("
				<< hp << "/" << maxHp << ")\n";
		}
		else if (item.type == ItemType::ManaPotion) {
			int restored = std::min(item.potency, maxMana - mana);
			mana += restored;
			std::cout << "  Used " << item.name << "! Restored " << restored << " Mana. ("
				<< mana << "/" << maxMana << ")\n";
		}

		items_.erase(items_.begin() + static_cast<ptrdiff_t>(index));
		return true;
	}

	size_t Size() const { return items_.size(); }
	const Item& GetItem(size_t index) const { return items_[index]; }

private:
	std::vector<Item> items_;
};
