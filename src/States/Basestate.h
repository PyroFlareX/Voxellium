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
	virtual void lateUpdate(Camera* cam) = 0;


	virtual void render(Renderer* renderer) = 0;

	virtual PlayerController& getPlayer() = 0;

    virtual ~Basestate() = default;
protected:
	Application& app;
	PlayerController pcontroller;
};


#endif // BASESTATE_H
