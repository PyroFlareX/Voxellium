#pragma once

#include "Basestate.h"

#include "../Application.h"
#include <Engine.h>
#include "../Controllers/PlayerController.h"

class Worldstate : public Basestate
{
public:
    Worldstate(Application& app);

	void createObject(bs::Transform& t, const std::string& name);
	void removeObject(const std::string& name);

    bool input(float dt) override;
    void update(float dt) override;
	void lateUpdate(Camera& cam) override;
	void render(Renderer& renderer) override;

	PlayerController& getPlayer() override;

    ~Worldstate() override;
	
private:

	std::vector<bs::GameObject> m_gameObjects;
	PlayerController m_player;
	
	Input::Inputs vInput;
};
