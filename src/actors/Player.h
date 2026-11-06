#pragma once

#include "../headers/classes.h"
#include "../headers/helpers.h"

namespace PlayerActor {

	inline float jumpBufferDuration = 0.12f;
	inline float jumpBufferRemaining = 0.0f;

	inline float groundNormalThreshold = 0.35f;

	inline bool jumpActionWasDown = false;
	inline bool grounded = false;
	inline glm::vec3 groundNormal(0.0f, 1.0f, 0.0f);
	inline float groundNormalUpDot = 0.0f;

	inline bool rampUphillBlock = false;
	inline glm::vec3 rampUphillDownhillDirection(0.0f);

	inline camera cameraState;
	inline bool cameraInitialized = false;
	inline float cameraEyeHeight = 1.7f;
	inline float cameraFollowSpeed = 16.0f;

	inline float lookSensitivity = 0.001f;

	inline float defaultMaximumSpeed = 450.0f;

	inline float jumpHeight = 150.0f;
	inline float jumpGravityMultiplier = 7.f;

	inline bool shiftWasDown = false;
	inline bool slideInputSuppressedUntilRelease = false;
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
	inline float slideAirTurnDegreesPerSecond = 360.0f;
	inline float slideCarryLandingDecayDuration = 0.04f;
	inline float slideCarryLandingDecayRemaining = 0.0f;
	inline bool slideCarryLandingDecayStarted = false;
	inline float slideCarryLandingStartSpeed = 0.0f;
	inline float slideJumpReleaseWindowDuration = 0.12f;
	inline float slideJumpReleaseWindowRemaining = 0.0f;

	inline float releasedSlideSpeed = 0.0f;
	inline glm::vec3 releasedSlideDirection(0.0f);

	inline bool standingScaleInitialized = false;
	inline glm::vec3 standingScale(1.0f);

	inline float speedFovOffset = 0.0f;
	inline float speedFovIncrease = 5.0f;
	inline float speedFovReferenceMultiplier = 1.5f;
	inline float speedFovChangeSpeed = 80.0f;

	inline bool crouched = false;
	inline EntityID crouchPlayerID = 0;
	inline float slideCrouchScale = 0.50f;
    inline float crouchAmount = 0.0f;
    inline float crouchTransitionDuration = 0.18f;
    inline float crouchedMoveSpeedMultiplier = 0.55f;

	inline float coyoteDuration = 0.25f;
    inline float coyoteRemaining = 0.0f;

	void Control(const bool *keyboard, float dt, Object &player);
	void UpdateCamera(Object &player, float dt);
	void ResetJumpState();
	void QueueJumpInput();
	void BeginCollisionStep();
	void RegisterCollision(Object &a, Object &b, const Contact &contact);
}
