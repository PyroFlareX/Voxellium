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
