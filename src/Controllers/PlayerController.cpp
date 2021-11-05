#include "PlayerController.h"
#include <iostream>

PlayerController::PlayerController()
{
	transform.pos = bs::vec3(0.0f, 2.0f, 0.0f);
	transform.rot = bs::vec3(0.0f, 0.0f, 0.0f);
	velocity = bs::vec3(0.0f, 0.0f, 0.0f);

	getCurrentCamera().follow(transform);
}

void PlayerController::getInput(Input::Inputs vInput)
{
	auto& io = ImGui::GetIO();

	bs::vec3 change = glm::vec3(0.0f);
	float speed = 50.0f;

	change.x += glm::cos(glm::radians(transform.rot.y)) * speed * vInput.AD;
	change.z += glm::sin(glm::radians(transform.rot.y)) * speed * vInput.AD;
		
	//go up
	if (vInput.space)
	{
		//jump();
		velocity.y += 3 * speed;
	}
	// go down
	if (vInput.shift)
	{
		velocity.y-= 5;
	}

	//Backwards
	if (vInput.WS < 0)
	{
		change.x += -glm::cos(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
		change.z += -glm::sin(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
	}

	//accelerate
	if (vInput.lctrl)
	{
		speed *= 3.0f;
	}

	//Forward
	if (vInput.WS > 0)
	{
		change.x += -glm::cos(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
		change.z += -glm::sin(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
	}

	velocity += change;

	//mouse movement
	//transform.rot.x -= vInput.mouseUD * 0.05f;
	//transform.rot.y += vInput.mouseLR * 0.05f;

	transform.rot.x = io.MouseDelta.y * 0.05f;
	transform.rot.y = io.MouseDelta.x * 0.05f;
}

Camera& PlayerController::getCurrentCamera() 
{
	// if (currentCamera == 0) {
	//     return debugCamera;
	// }
	// else {
	//     return gameCamera;
	// }

	return gameCamera;
}

bs::Transform& PlayerController::getTransform()
{
	return transform;
};

void PlayerController::update(float dt)
{
	transform.pos += velocity * dt;
	velocity = bs::vec3(0.0f);
}

