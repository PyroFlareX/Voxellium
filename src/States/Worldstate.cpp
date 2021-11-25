#include "Worldstate.h"

Worldstate::Worldstate(Application& app) : Basestate(app)
{
	m_playerView.pos = {0.0f, 2.0f, -2.0f};
	m_playerView.rot = {0.0f, 180.0f, 0.0f};
	m_playerView.scale = {0.0f, 0.0f, 0.0f};
	m_playerView.origin = {0.0f, 0.0f, 0.0f};

	constexpr auto min = 0;
	constexpr auto max = 1;

	std::cout << "Generating Chunks and Meshes\n";

	const auto currentJobs = jobSystem.remainingJobs();
	u32 jobsScheduled = 0;

	for(auto chunk_x = min; chunk_x < max; ++chunk_x)
	{
		for(auto chunk_y = 0; chunk_y < max; ++chunk_y)
		{
			for(auto chunk_z = min; chunk_z < max; ++chunk_z)
			{
				const pos_xyz chunk_pos(chunk_x, chunk_y, chunk_z);
				auto* world_ptr = &m_world;
				const auto generateChunk = jobSystem.createJob([world_ptr, chunk_pos](Job j)
				{
					auto& chunk = world_ptr->getChunkAt(chunk_pos);
					for(auto z = 0; z < CHUNK_SIZE; z+=1)
					{
						for(auto y = 0; y < CHUNK_SIZE; y+=1)
						{
							for(auto x = 0; x < CHUNK_SIZE; x+=1)
							{
								const pos_xyz worldpos(chunk_pos.x * CHUNK_SIZE + x, 
														chunk_pos.y * CHUNK_SIZE + y, 
														chunk_pos.z * CHUNK_SIZE + z);

								chunk.setBlockAt({x, y, z}, y % 2);
							}
						}
					}
					chunk.checkIfEmpty();

					const bool result = world_ptr->getMeshManager().cacheChunk(chunk);

					std::cout << "Caching Chunk result: " << (result ? "true" : "false") << "\n";
				});
				jobSystem.schedule(generateChunk, false);
				jobsScheduled += 1;
			}
		}
	}

	std::cout << "Jobs Scheduled: " << jobsScheduled << "\n";
	while(jobSystem.backgroundJobs() > currentJobs)	{	}
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
	ImGui::SetNextWindowSize({250.0f, 100.0f});
	if(ImGui::Begin("Debug UI", nullptr, windowflag))
	{
		const auto& cam = app.getCamera();
		ImGui::Text("Player Pos:\n X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.pos.x, cam.pos.y, cam.pos.z);
		ImGui::Text("Player Rot:\n X:%0.3f, Y:%0.3f, Z:%0.3f\n", cam.rot.x, cam.rot.y, cam.rot.z);
		
		
		const auto& io = ImGui::GetIO();
		ImGui::Text("\nFPS: %0.2f", io.Framerate);
	}
	ImGui::End();
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

	static int prevRenderedChunks = 0;
	if(prevRenderedChunks != m_world.getMeshManager().getNumChunks())
	{
		renderer.recreateChunkDrawCommands(m_world.getMeshManager().getChunkDrawData());
		prevRenderedChunks = m_world.getMeshManager().getNumChunks();
		std::cout << "Drawing: " << prevRenderedChunks << "\n";
	}
}
