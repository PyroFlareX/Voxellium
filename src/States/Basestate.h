#ifndef BASESTATE_H
#define BASESTATE_H

#include "../Renderers/Renderer.h"
#include "../Controllers/PlayerController.h"

class Application;
class Camera;

class Basestate
{
public:
	Basestate(Application& app)	:	app(app)	{};

    virtual bool input(float dt) = 0;
    virtual void update(float dt) = 0;
	virtual void lateUpdate(Camera& cam) = 0;

	virtual void render(Renderer& renderer) = 0;

	PlayerController& getPlayer()
	{
		return player_controller;
	}

    virtual ~Basestate() = default;
protected:
	Application& app;
	PlayerController player_controller;
};


#endif // BASESTATE_H
