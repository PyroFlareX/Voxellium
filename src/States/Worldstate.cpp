#include "Worldstate.h"


Worldstate::Worldstate(Application& app) : Basestate(app)
{
	m_playerView.pos = {0.0f, 0.0f, 0.0f};
	m_playerView.rot = {0.0f, 0.0f, 0.0f};
	m_playerView.scale = {0.0f, 0.0f, 0.0f};
	m_playerView.origin = {0.0f, 0.0f, 0.0f};
}

Worldstate::~Worldstate()
{
	app.getCamera().follow(app.getCamera());
}

void Worldstate::input(float dt)
{
	ImGui::NewFrame();
	vInput = Input::getInput(dt);
	const auto& io = ImGui::GetIO();

	const float movementSpeed = 5.0f;
	
}

void Worldstate::update(float dt)
{
	constexpr auto windowflag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

	if(ImGui::Begin("Main Menu", nullptr, windowflag))
	{
		const auto& cam = app.getCamera();
		ImGui::Text("Player Pos: X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.pos.x, cam.pos.y, cam.pos.z);
		ImGui::Text("Player Rot: X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.rot.x, cam.rot.y, cam.rot.z);
	}

	ImGui::End();

	//Chunk update list
	auto* world = &m_world;
	for(const auto& [chunk_pos, chunk] : *m_world.getWorldMap())
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
	cam.follow(m_playerView);
}

void Worldstate::render(Renderer& renderer)
{		
	for (auto& obj : m_gameObjects)
	{
		obj.getCurrentTransform();
		renderer.drawObject(obj);
	}

	static bool ran = false;
	const pos_xyz chunk_pos(0, 0, 0);
	if(m_world.getChunkAt(chunk_pos).getChunkMesh().has_value() && !ran)
	{
		bs::Transform t;
		t.pos = chunk_pos * CHUNK_SIZE;
		std::string modelname("chunk_" + 
			std::to_string(chunk_pos.x) + 
			std::to_string(chunk_pos.y) + 
			std::to_string(chunk_pos.z));

		bs::GameObject chunk(t, modelname);

		m_gameObjects.emplace_back(chunk);

		std::cout << "Added chunk at (0,0,0)\n";

		ran = true;
	}
}
