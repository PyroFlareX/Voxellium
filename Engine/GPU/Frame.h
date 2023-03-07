#pragma once

#include "../Types/Types.h"

namespace bs
{
	/**
	 * @brief Frame structure
	 * Contains data about the frame which will be passed to the shader
	 */
	struct Frame
	{
		mat4 proj;			//Camera Matrix
		mat4 view;			//View Matrix

		vec2i resolution;	//Pixel resolution of the image

		int frame_id;		//The ID of the frame
		int frame_stride;	//The index of the swapchain

		float time;			//in seconds
		float dt;			//in ms
		float framerate;	//fps
		
	};
} // namespace bs
