#pragma once

#include "../Types/Block.h"
#include <Registries/ResourceManager.h>

class BlockRegistry
{
public:
	BlockRegistry();

	void addBlock(Block&& block);

	Block& getBlock(const block_t blockID);
	const Block& getBlock(const block_t blockID) const;

	size_t getBlockCount() const;

	
private:

};