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
	auto& io = ImGui::GetIO();
	
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

	//Chunk update list
	const auto worldmap = m_world.getWorldMap();
	auto* world = &m_world;
	for(const auto& [chunk_pos, chunk] : *worldmap)
	{
		if(chunk.isEmpty())
		{
			continue;
		}

		if(chunk.needsMesh())
		{
			const auto makeChunkMesh = jobSystem.createJob([chunk_pos, world](Job j)
			{
				auto* w = world;
				generateMeshFor(*w, chunk_pos);
				std::cout << "Generated Mesh!\n";
				
				bs::vk::Model chunkModel(w->getChunkAt(chunk_pos).getChunkMesh().value_or(bs::Mesh()),
										bs::asset_manager->getTextureMutable(0).getDevice());

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
	//Add the chunks to be rendered to a list
}

void Worldstate::render(Renderer& renderer)
{		
	for (auto& obj : m_gameObjects)
	{
		obj.getCurrentTransform();
		renderer.drawObject(obj);
	}
}
