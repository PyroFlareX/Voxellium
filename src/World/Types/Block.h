#pragma once

#include "AliasTypes.h"

//Block; Holds the data for a block type
// Properties for id, localization, textures, rendering,
// and whatever else will be queued from here.
class Block
{
public:
	Block(block_t block_id, const std::string& loc_name); //adding more args later

	//Returns the ID of the block
	block_t getType() const noexcept;
	//Returns the string identifier of the block (the localization string)
	const std::string& getNameString() const noexcept;

	//Returns the texture...

	//Property accessor functions

	//Then any other thing about the block that should be accessed
	
private:
	const block_t m_id;
	const std::string m_name;
};