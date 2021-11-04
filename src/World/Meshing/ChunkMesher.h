#pragma once

#include <memory>
#include <Resources/Mesh.h>

#include "../Types/AliasTypes.h"

class World;

std::shared_ptr<bs::Mesh> generateMeshFor(World& world, const pos_xyz& chunk_coord);