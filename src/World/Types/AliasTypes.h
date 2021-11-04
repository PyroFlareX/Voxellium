#pragma once

#include <Types/Types.h>

using block_t = u8;
using pos_xyz = bs::vec3i;

namespace std
{
	template<>
	struct hash<pos_xyz>
	{
		size_t operator()(const pos_xyz& vec) const noexcept
		{
			std::hash<decltype(vec.x)> hasher;

			auto hash1 = hasher(vec.x);
			auto hash2 = hasher(vec.y);
			auto hash3 = hasher(vec.z);

			return std::hash<decltype(vec.x)>{}((hash1 ^ (hash2 << hash3) ^ hash3));
		}
	};
} // For adding the pos_xyz hash
