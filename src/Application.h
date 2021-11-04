#ifndef APPLICATION_H
#define APPLICATION_H

#include <vector>
#include <memory>
#include <iostream>

#include <Engine.h>
#include "Renderers/Renderer.h"
#include "Camera.h"
#include "States/Basestate.h"


class Application
{
public:
    Application();
    ~Application();

	//Main loop
    void RunLoop();

    //State Stuff

	void pushState(std::unique_ptr<Basestate> state)
	{
		m_states.emplace_back(std::move(state));
	}
	void pushBackState(std::unique_ptr<Basestate> state)
	{
		auto change = [&]()
		{
			auto* current = m_states.back().release();
			m_states.back() = (std::move(state));
			m_states.emplace_back(std::unique_ptr<Basestate>(current));
		};
		m_statechanges.emplace_back(change);
	}

    void popState();
    void handleEvents();
	// Camera& getCam() 
	// {
	// 	return m_camera;
	// }
	
	void requestClose()
	{
		shouldClose = true;
	}

protected:

private:
    std::unique_ptr<Basestate>& currentState();

	// Windowing Context
	bs::Context* m_context;
	bs::vk::FramebufferData framebufdata[2];
	// Device Context
	bs::Device* m_device;

	//Camera m_camera;
    std::vector<std::unique_ptr<Basestate>> m_states;
	Renderer* m_renderer;
	
	bool shouldClose = false;
	
	std::vector<std::function<void()>> m_statechanges;
};


#endif // APPLICATION_H
