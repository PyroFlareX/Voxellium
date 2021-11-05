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
}
