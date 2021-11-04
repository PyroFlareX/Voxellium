#pragma once

#include <Engine.h>

class Camera : public bs::Transform
{
public:
    Camera();

	//Get the view matrix of the camera
	bs::mat4 getViewMatrix() const;
	//Get the projection matrix for the scene
	bs::mat4 getProjMatrix() const;

	//Capture a transform to follow
	void follow(bs::Transform& entity);
	//Self evident
	void update();
	
	
	~Camera() = default;
private:
	float lerp;
	
	//Mode 0 is default, 1 is Left Eye, 2 is Right Eye	//Or now ig for other stuff THE BOBSTER does
	int mode;

	bs::mat4 proj;

	bs::Transform* entityPos;
};

