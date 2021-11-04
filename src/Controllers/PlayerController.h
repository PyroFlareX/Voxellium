#pragma once

#include <Engine.h>
#include "../Camera.h"
#include "Controller.h"

class PlayerController //: public bs::Transform //: public Controller
{
public:
    PlayerController();

    void update(float deltatime);

    void getInput(Input::Inputs inputs);

    Camera& getCurrentCamera();

    bs::Transform& getTransform();

    short country = 0;

    ~PlayerController();
private:
	Camera debugCamera;
	Camera gameCamera;

    // set 0 for debugcamera, set 1 for gamecamera
    int currentCamera = 1;
    bool fixedMapFlag = false;
    bs::Transform transform;
    
    bs::vec3 velocity;
};
