#pragma once

#include "../headers/classes.h"

namespace PlayerActor {

	inline camera cameraState;
	inline bool cameraInitialized = false;
	inline float cameraEyeHeight = 1.7f;
	inline float cameraFollowSpeed = 16.0f;
	inline float lookSensitivity = 0.002f;
	inline float defaultMaximumSpeed = 300.0f;

	void Control(const bool *keyboard, float dt, Object &player);
	void UpdateCamera(Object &player, float dt);
}
