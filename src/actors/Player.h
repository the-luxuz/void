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

	inline bool shiftWasDown = false;
	inline bool movingBeforeSlide = false;
	inline bool sliding = false;

	inline float normalMaximumSpeed = 0.0f;
    inline float slideCurrentSpeed = 0.0f;
    inline float slideSpeedCeiling = 0.0f;
    inline glm::vec3 slideDirection(0.0f);

	inline glm::vec3 slideVelocityDirection(0.0f);
	inline bool slideCarryActive = false;
	inline float slideCarrySpeed = 0.0f;
	inline glm::vec3 slideCarryDirection(0.0f);
	inline float slideAirTurnDegreesPerSecond = 180.0f;
	inline bool standingScaleInitialized = false;
	inline glm::vec3 standingScale(1.0f);
	inline float slideElapsed = 0.0f;
	inline float slideFovOffset = 0.0f;
	inline float slideFovIncrease = 7.0f;
	inline float slideFovRiseDuration = 0.3f;
	inline float slideFovReturnDuration = 0.8f;
	inline float slideFovChangeSpeed = 80.0f;

	inline bool crouched = false;
	inline EntityID crouchPlayerID = 0;
	inline float slideCrouchScale = 0.70f;
    inline float crouchAmount = 0.0f;
    inline float crouchTransitionDuration = 0.18f;
    inline float crouchedMoveSpeedMultiplier = 0.55f;

	inline float coyoteDuration = 0.25f;
    inline float coyoteRemaining = 0.0f;

	void Control(const bool *keyboard, float dt, Object &player);
	void UpdateCamera(Object &player, float dt);
	void ResetJumpState();
	void BeginCollisionStep();
	void RegisterCollision(const Object &a, const Object &b, const Contact &contact);
}
