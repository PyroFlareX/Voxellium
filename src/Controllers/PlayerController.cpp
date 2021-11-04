#include "PlayerController.h"
#include <iostream>

PlayerController::PlayerController()
{
	
	getCurrentCamera().follow(transform);


	// getCurrentCamera().pos = bs::vec3(0.0f, 5.0f, 0.0f);
	// getCurrentCamera().rot = bs::vec3(90.0f, 0.0f, 0.0f);

	// for the bob
	// y is height
	transform.pos = bs::vec3(0.0f, 2.0f, 0.0f);
	transform.rot = bs::vec3(90.0f, 0.0f, 0.0f);
	velocity = bs::vec3(0.0f, 0.0f, 0.0f);

	
}

void PlayerController::getInput(Input::Inputs vInput)
{
	auto& io = ImGui::GetIO();

	if (currentCamera == 0)
	{
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
		transform.rot.x -= vInput.mouseUD * 0.05f;
		transform.rot.y += vInput.mouseLR * 0.05f;
	}
	else 
	{
		bs::vec3 change = glm::vec3(0.0f);
		float speed = 10.0f;
		float mouseSpeed = 0.001f;

		// I only put these here because I cba on the refix
		static bool fixedMapFlag = false;
		static glm::vec3 dragHoldWorld;

		if(io.MouseDown[2] && !fixedMapFlag)
		{
			fixedMapFlag = true;
			//std::cout << fixedMapFlag << std::endl;
			dragHoldWorld = transform.pos;
		} else if(!io.MouseDown[2] && fixedMapFlag) {
			fixedMapFlag = io.MouseDown[2];
			//std::cout << fixedMapFlag << std::endl;
		}

		change.x += glm::cos(glm::radians(transform.rot.y)) * speed * vInput.AD;
		change.z += glm::sin(glm::radians(transform.rot.y)) * speed * vInput.AD;
		
		//go up
		if (vInput.space)
		{
			//jump();
			velocity.y += 5;
		}
		// go down
		if (vInput.shift)
		{
			velocity.y -= 5;
		}

		//Backwards
		if (vInput.WS < 0)
		{
			change.x += -glm::cos(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
			change.z += -glm::sin(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
		}

		//Forward
		if (vInput.WS > 0)
		{
			change.x += -glm::cos(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
			change.z += -glm::sin(glm::radians(transform.rot.y + 90)) * speed * vInput.WS;
		}

		velocity += change;

		// B: ctrl-A delete isn't making things cleaner
		if(fixedMapFlag)
		{
			//std::cout << "FIXED" << std::endl;
			// B: this needs to account for your screen's ratio but only does in a hard coded way
			transform.pos.z = glm::clamp(dragHoldWorld.z + vInput.mouseUD * (transform.pos.y * 1.5f), -8.0f, 8.0f);
			transform.pos.x = glm::clamp(dragHoldWorld.x +  + vInput.mouseLR * (transform.pos.y * 3.0f), -21.0f, 21.0f);
		} else {
			//std::cout << "MOBILE" << std::endl;
			transform.pos.z = glm::clamp(transform.pos.z, -8.0f, 8.0f)  + glm::clamp(vInput.mouseUD * (transform.pos.y / 5) * mouseSpeed, -1.0f, 1.0f);
			transform.pos.x = glm::clamp(transform.pos.x, -21.0f, 21.0f)  + glm::clamp((vInput.mouseLR * (transform.pos.y / 5)) * mouseSpeed, -1.0f, 1.0f); //TEMPORARY UNTIL MAP WRAPPING IS IMPLEMENTED
		}

		transform.pos.y = glm::clamp(float(transform.pos.y + (vInput.zoom * (transform.pos.y / 3.0f))), 0.35f, 10.0f);
		Input::resetZoom();
	}
	
}

Camera& PlayerController::getCurrentCamera() 
{
	// if (currentCamera == 0) {
	//     return debugCamera;
	// }
	// else {
	//     return gameCamera;
	// }

	return debugCamera;
}

bs::Transform& PlayerController::getTransform()
{
	return transform;
};

void PlayerController::update(float dt)
{
	transform.pos += velocity * dt;
	velocity = bs::vec3(0.0f);

	const bs::vec2 lowhigh(3.5f, 10.0f);

	float rotscale = (transform.pos.y - lowhigh.x) / (lowhigh.y - lowhigh.x);
	rotscale = 1 - rotscale;

	const float lowpitchangle = -10.0f; // -15.0f works well cinematically

	transform.rot.x = lowpitchangle * rotscale + 90.0f;
}

PlayerController::~PlayerController()
{

}
