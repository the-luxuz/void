#pragma once

#include "../headers/classes.h"

namespace PlayerActor {

	inline float jumpBufferDuration = 0.12f;
	inline float groundNormalThreshold = 0.5f;

	inline bool jumpKeyWasDown = false;
	inline bool grounded = false;
	inline float jumpBufferRemaining = 0.0f;

	inline camera cameraState;
	inline bool cameraInitialized = false;
	inline float cameraEyeHeight = 1.7f;
	inline float cameraFollowSpeed = 16.0f;
	inline float lookSensitivity = 0.002f;
	inline float defaultMaximumSpeed = 450.0f;
	inline float jumpHeight = 150.0f;
	inline float jumpMinimumHeightPercent = 1.0f;
	inline float jumpHoldDuration = 0.08f;
	inline float jumpGravityMultiplier = 7.f;
	inline float jumpReleaseBrakeDuration = 0.12f;
	inline float jumpReleaseBrakeMultiplier = 1.5f;
	inline float jumpHoldRemaining = 0.0f;
	inline float jumpHoldForceAcceleration = 0.0f;
	inline float jumpReleaseBrakeRemaining = 0.0f;

	void Control(const bool *keyboard, float dt, Object &player);
	void UpdateCamera(Object &player, float dt);
	void ResetJumpState();
	void BeginCollisionStep();
	void RegisterCollision(const Object &a, const Object &b, const Contact &contact);
}
