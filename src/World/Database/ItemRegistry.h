#pragma once

#include "../Types/Item.h"
#include <Registries/ResourceManager.h>

using item_t = int;

class ItemRegistry
{
public:
	ItemRegistry();

	void addItem(Item&& item);

	Item& getItem(const item_t itemID);
	const Item& getItem(const item_t itemID) const;

	size_t getItemCount() const;

private:
	
};