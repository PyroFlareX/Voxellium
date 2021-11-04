#ifndef Menustate_H
#define Menustate_H

#include "Basestate.h"

#include "../Application.h"
#include <Engine.h>
#include "../Controllers/PlayerController.h"

class Menustate : public Basestate
{
public:
    Menustate(Application& app);

	void createObject(bs::Transform& t, const std::string& name);
	void removeObject(std::string name);

    bool input(float dt) override;
    void update(float dt) override;
	void lateUpdate(Camera* cam) override;
	void render(Renderer* renderer) override;

	PlayerController& getPlayer() override;

    ~Menustate() override;
protected:


private:

	std::vector<bs::GameObject> m_gameObjects;
	PlayerController m_player;
	
	Input::Inputs vInput;
};

#endif // Menustate_H
