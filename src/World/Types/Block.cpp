#include "Block.h"

Block::Block(block_t block_id, const std::string& loc_name)	:	m_id(block_id), m_name(loc_name)
{
	//
}

//Returns the ID of the block
block_t Block::getType() const noexcept
{
	return m_id;
}

//Returns the string identifier of the block (the localization string)
const std::string& Block::getNameString() const noexcept
{
	return m_name;
}