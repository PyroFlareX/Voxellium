#include "Worldstate.h"


Worldstate::Worldstate(Application& app) : Basestate(app)
{

}

Worldstate::~Worldstate()
{

}

bool Worldstate::input(float dt)
{
	ImGui::NewFrame();
	vInput = Input::getInput(dt);
	const auto& io = ImGui::GetIO();

	getPlayer().getInput(vInput);
	
	return false;
}

void Worldstate::update(float dt)
{
	static uint8_t menuIndex = 1;
	constexpr auto windowflag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

	switch(menuIndex)
	{
		case 1:
		{
			
			break;
		}
		case 2:
		{
			
			break;
		}
	}

	//Input
	getPlayer().update(dt);

	//Chunk update list
	const auto worldmap = m_world.getWorldMap();
	auto* world = &m_world;
	for(const auto& [chunk_pos, chunk] : *worldmap)
	{
		/*if(chunk.isEmpty())
		{
			continue;
		}*/

		if(chunk.needsMesh())
		{
			const auto makeChunkMesh = jobSystem.createJob([chunk_pos, world](Job j)
			{
				auto* w = world;
				generateMeshFor(*w, chunk_pos);
				//std::cout << "Generated Mesh!\n";
				
				const auto& m = w->getChunkAt(chunk_pos).getChunkMesh();
				if(!m.has_value())
				{
					return;
				}

				bs::vk::Model chunkModel(*m, bs::asset_manager->getTextureMutable(0).getDevice());

				std::string modelname("chunk_" + 
							std::to_string(chunk_pos.x) + 
							std::to_string(chunk_pos.y) + 
							std::to_string(chunk_pos.z));
				bs::asset_manager->addModel(chunkModel, std::move(modelname));
			});
			jobSystem.schedule(makeChunkMesh, false);
		}
	}
}

void Worldstate::lateUpdate(Camera& cam)
{
	
}

void Worldstate::render(Renderer& renderer)
{		
	for (auto& obj : m_gameObjects)
	{
		obj.getCurrentTransform();
		renderer.drawObject(obj);
	}

	//Add the chunks to be rendered to a list
	//for(int z = 0; z < 2; z+=1)
	{
		//for(int y = 0; y < 2; y+=1)
		{
			//for(int x = 0; x < 2; x+=1)
			{
				//const pos_xyz chunk_pos(x, y, z);
				const pos_xyz chunk_pos(0, 0, 0);

				if(m_world.getChunkAt(chunk_pos).getChunkMesh().has_value())
				{
					const pos_xyz world_pos = chunk_pos * CHUNK_SIZE;
					bs::Transform t;
					t.pos = world_pos;

					std::string modelname("chunk_" + 
						std::to_string(chunk_pos.x) + 
						std::to_string(chunk_pos.y) + 
						std::to_string(chunk_pos.z));

					bs::GameObject chunk(t, modelname);
					renderer.drawObject(chunk);
				}

			}
		}
	}
}
