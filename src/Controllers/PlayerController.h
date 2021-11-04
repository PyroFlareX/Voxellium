#pragma once

#include <Engine.h>
#include "../Camera.h"
#include "Controller.h"

class PlayerController
{
public:
    PlayerController();
    ~PlayerController() = default;

    void getInput(Input::Inputs inputs);

    void update(float deltatime);

    Camera& getCurrentCamera();

    bs::Transform& getTransform();

private:
	Camera gameCamera;

    bs::Transform transform;
    
    bs::vec3 velocity;
};
