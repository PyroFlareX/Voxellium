#include "Menustate.h"

#include "Worldstate.h"

Menustate::Menustate(Application& app) : Basestate(app)
{
	
}

Menustate::~Menustate()
{

}

bool Menustate::input(float dt)
{
	ImGui::NewFrame();
	vInput = Input::getInput(dt);
	auto& io = ImGui::GetIO();

	return false;
}


void Menustate::update(float dt)
{
	static uint8_t menuIndex = 1;
	constexpr auto windowflag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

	if(ImGui::Begin("Main Menu", nullptr, windowflag))
	{
		//ImGui::Window

		if(ImGui::Button("Start"))
		{
			app.pushState(std::make_unique<Worldstate>(Worldstate(app)));
		}
	}

	ImGui::End();

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
}

void Menustate::lateUpdate(Camera& cam)
{

}

//called in Application.cpp loop
void Menustate::render(Renderer& renderer)
{		
	for (auto& obj : m_gameObjects)
	{
		obj.getCurrentTransform();
		renderer.drawObject(obj);
	}
}

