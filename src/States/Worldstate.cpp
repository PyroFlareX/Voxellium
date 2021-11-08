#include "Worldstate.h"

// #include <mutex>
// std::mutex g_obj_guard;

Worldstate::Worldstate(Application& app) : Basestate(app)
{
	m_playerView.pos = {0.0f, 2.0f, -2.0f};
	m_playerView.rot = {0.0f, 180.0f, 0.0f};
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

	constexpr auto W = GLFW_KEY_W;
	constexpr auto A = GLFW_KEY_A;
	constexpr auto S = GLFW_KEY_S;
	constexpr auto D = GLFW_KEY_D;
	constexpr auto UP = GLFW_KEY_SPACE;
	const float movementSpeed = dt * 6.0f; // The literal is the units moved per second
	const float mouseSensitivity = 0.7f;

	//Input handling, modifying the transform
	auto& pos = m_playerView.pos;
	auto& rot = m_playerView.rot;
	rot.z = 0.0f;
	//Forward - Backwards
	if(io.KeysDown[W] ^ io.KeysDown[S])
	{
		const int dir = (io.KeysDown[W]) ? 1 : -1;
		pos.x += -glm::cos(glm::radians(rot.y + 90)) * movementSpeed * dir;
		pos.z += -glm::sin(glm::radians(rot.y + 90)) * movementSpeed * dir;
	}
	//Left - Right
	if(io.KeysDown[A] ^ io.KeysDown[D])
	{
		const int dir = (io.KeysDown[D]) ? 1 : -1;
		pos.x += glm::cos(glm::radians(rot.y)) * movementSpeed * dir;
		pos.z += glm::sin(glm::radians(rot.y)) * movementSpeed * dir;
	}
	//Up
	if(io.KeysDown[UP])
	{
		pos.y += movementSpeed;
	}
	//Down
	if(io.KeyShift)
	{
		pos.y -= movementSpeed;
	}
	//For mouse controls
	const auto& mouseChange = io.MouseDelta;
	rot += bs::vec3(mouseChange.y, mouseChange.x, 0.0f) * mouseSensitivity;
}

void Worldstate::update(float dt)
{
	constexpr auto windowflag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
	ImGui::SetNextWindowSize({250.0f, 75.0f});
	if(ImGui::Begin("Debug UI", nullptr, windowflag))
	{
		const auto& cam = app.getCamera();
		ImGui::Text("Player Pos:\n X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.pos.x, cam.pos.y, cam.pos.z);
		ImGui::Text("Player Rot:\n X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.rot.x, cam.rot.y, cam.rot.z);
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
				auto& chunk = w->getChunkAt(chunk_pos);
				generateMeshFor(*w, chunk);
				
				const auto& mesh = chunk.getChunkMesh();
				if(!mesh.has_value())
				{
					//return;
				}
				else
				{
					bs::asset_manager->addModel(bs::vk::Model(*mesh, bs::asset_manager->getTextureMutable(0).getDevice()),
						std::string("chunk_" + std::to_string(chunk_pos.x) + std::to_string(chunk_pos.y) + 
							std::to_string(chunk_pos.z)));
				}
			});
			//jobSystem.schedule(makeChunkMesh, false);
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

		m_gameObjects.emplace_back(t, modelname);
		m_gameObjects.back().material.texture_id = 2;

		std::cout << "Added chunk at (" << chunk_pos.x << ", " << chunk_pos.y << ", " << chunk_pos.z << ")\n";
		ran = true;
	}
}
