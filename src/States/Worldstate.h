#pragma once

#include "../Application.h"

#include "../World/World.h"

class Worldstate : public Basestate
{
public:
    Worldstate(Application& app);
	~Worldstate() override;

	void createObject(bs::Transform& t, const std::string& name);
	void removeObject(const std::string& name);

    void input(float dt) override;
    void update(float dt) override;
	void lateUpdate(Camera& cam) override;
	void render(Renderer& renderer) override;
	
private:
	std::vector<bs::GameObject> m_gameObjects;
	
	Input::Inputs vInput;

	World m_world;

	bs::Transform m_playerView;
};
