#include "Player.h"

#include "../headers/helpers.h"

namespace PlayerActor {

    void ResetJumpState() {

        jumpKeyWasDown = false;

        grounded = false;

        jumpBufferRemaining = 0.0f;
        wheelJumpIntentRemaining = 0.0f;

        shiftWasDown = false;
        slideInputSuppressedUntilRelease = false;
        movingBeforeSlide = false;
        sliding = false;
        normalMaximumSpeed = 0.0f;
        slideCurrentSpeed = 0.0f;
        slideSpeedCeiling = 0.0f;
        slideDirection = glm::vec3(0.0f);
        slideVelocityDirection = glm::vec3(0.0f);
        slideCarryActive = false;
        slideCarrySpeed = 0.0f;
        slideCarryDirection = glm::vec3(0.0f);
        slideCarryLandingGraceRemaining = 0.0f;
        slideCarryLandingGraceStarted = false;
        slideJumpReleaseWindowRemaining = 0.0f;
        releasedSlideSpeed = 0.0f;
        releasedSlideDirection = glm::vec3(0.0f);
        crouched = false;
        crouchAmount = 0.0f;
        standingScaleInitialized = false;
        crouchPlayerID = 0;
        standingScale = glm::vec3(1.0f);
        speedFovOffset = 0.0f;

        coyoteRemaining = 0.0f;
    }

    void BeginCollisionStep() {

        grounded = false;
    }

    void RegisterCollision(const Object &a, const Object &b, const Contact &contact) {

        if (!contact.hit || a.collider.isTrigger || b.collider.isTrigger)
            return;

        glm::vec3 up(0.0f, 1.0f, 0.0f);
        bool playerCollision = a.name == "Player" || b.name == "Player";
        bool wallCollision = std::abs(glm::dot(contact.normal, up)) < groundNormalThreshold;

        if (playerCollision && wallCollision) {
            if (sliding) {
                slideInputSuppressedUntilRelease = true;
                crouched = false;
            }

            sliding = false;
            movingBeforeSlide = false;
            slideCurrentSpeed = 0.0f;
            slideSpeedCeiling = 0.0f;
            slideCarryActive = false;
            slideCarrySpeed = 0.0f;
            slideCarryDirection = glm::vec3(0.0f);
            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);
        }

        if (a.name == "Player" && glm::dot(contact.normal, up) < -groundNormalThreshold) {
            grounded = true;
        }

        if (b.name == "Player" && glm::dot(contact.normal, up) > groundNormalThreshold) {
            grounded = true;
        }
    }

    void Control(const bool *keyboard, float dt, Object &player) {

        bool jumpKeyDown = keyboard[SDL_SCANCODE_SPACE];

        if (jumpKeyDown && !jumpKeyWasDown)
            jumpBufferRemaining = jumpBufferDuration;
        else if (jumpBufferRemaining > 0.0f)
            jumpBufferRemaining = std::max(0.0f, jumpBufferRemaining - dt);

        if (wheelJumpIntentRemaining > 0.0f) {
            jumpBufferRemaining = jumpBufferDuration;
            wheelJumpIntentRemaining = std::max(0.0f, wheelJumpIntentRemaining - dt);
        }

        jumpKeyWasDown = jumpKeyDown;

        if (!player.hasPhysics || player.physics.isStatic)
            return;

        Physics &physics = player.physics;

        if (physics.inverseMass <= 0.0f)
            return;

        if (!standingScaleInitialized || crouchPlayerID != player.id) {
            standingScale = player.scale;
            standingScaleInitialized = true;
            crouchPlayerID = player.id;
        }

        if (physics.maxHorizontalSpeed <= 0.0f)
            physics.maxHorizontalSpeed = defaultMaximumSpeed;

        float cosYaw = std::cos(cameraState.alpha);
        float sinYaw = std::sin(cameraState.alpha);

        glm::vec3 forward(cosYaw, 0.0f, sinYaw);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 moveDirection(0.0f);

        if (keyboard[SDL_SCANCODE_W])
            moveDirection += forward;

        if (keyboard[SDL_SCANCODE_S])
            moveDirection -= forward;

        if (keyboard[SDL_SCANCODE_A])
            moveDirection -= right;

        if (keyboard[SDL_SCANCODE_D])
            moveDirection += right;

        //-----------------------------------------------------------------------------
        // slide

        bool shiftDown = keyboard[SDL_SCANCODE_LSHIFT];
        bool shiftPressed = shiftDown && !shiftWasDown;
        bool hasMove = glm::length2(moveDirection) > 0.0001f;
        bool slideWasActive = sliding;
        float previousSlideCurrentSpeed = slideCurrentSpeed;
        glm::vec3 previousSlideVelocityDirection = slideVelocityDirection;

        if (shiftPressed)
            movingBeforeSlide = hasMove;

        shiftWasDown = shiftDown;

        if (!shiftDown) {
            slideInputSuppressedUntilRelease = false;
            crouched = false;
        } else if (slideInputSuppressedUntilRelease) {
            crouched = false;
        } else if (grounded) {
            crouched = true;
        }

        float previousAmount = crouchAmount;
        float targetAmount = crouched ? 1.0f : 0.0f;
        float step = dt / std::max(crouchTransitionDuration, 0.001f);

        crouchAmount += glm::clamp(targetAmount - crouchAmount, -step, step);

        player.scale = standingScale;
        player.scale.y = standingScale.y * (1.0f - (1.0f - slideCrouchScale) * crouchAmount);

        player.position.y -= std::abs(standingScale.y) * (1.0f - slideCrouchScale) * 0.5f * (crouchAmount - previousAmount);

        if (normalMaximumSpeed <= 0.0f)
            normalMaximumSpeed = physics.maxHorizontalSpeed;

        if (shiftDown && grounded && movingBeforeSlide && hasMove && !sliding) {

            sliding = true;
            slideCarryActive = false;

            slideCarrySpeed = 0.0f;

            slideCarryDirection = glm::vec3(0.0f);
            slideCarryLandingGraceRemaining = 0.0f;

            slideCarryLandingGraceStarted = false;
            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

            glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

            slideDirection = glm::length2(horizontalVelocity) > 0.0001f ? glm::normalize(horizontalVelocity) : glm::normalize(moveDirection);

            slideCurrentSpeed = std::max(glm::length(horizontalVelocity), normalMaximumSpeed * 1.5f);

            slideSpeedCeiling = slideCurrentSpeed;

            slideVelocityDirection = slideDirection;

        }

        if (!shiftDown) {

            sliding = false;

            movingBeforeSlide = false;

            slideCurrentSpeed = 0.0f;

            slideSpeedCeiling = 0.0f;
        }

        if (shiftDown) {
            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);
        } else if (slideWasActive) {
            slideJumpReleaseWindowRemaining = std::max(slideJumpReleaseWindowDuration, 0.0f);
            releasedSlideSpeed = previousSlideCurrentSpeed;
            releasedSlideDirection = previousSlideVelocityDirection;
        } else if (slideJumpReleaseWindowRemaining > 0.0f) {
            slideJumpReleaseWindowRemaining = std::max(0.0f, slideJumpReleaseWindowRemaining - dt);
        }

        if (slideJumpReleaseWindowRemaining <= 0.0f) {
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);
        }

        float referenceSpeed = std::max(normalMaximumSpeed * speedFovReferenceMultiplier, 1.0f);
        glm::vec2 horizontalVelocity(physics.velocity.x, physics.velocity.z);
        float speedRatio = glm::clamp(glm::length(horizontalVelocity) / referenceSpeed, 0.0f, 1.0f);
        float targetFovOffset = speedFovIncrease * speedRatio;
        float maxFovStep = speedFovChangeSpeed * dt;
        speedFovOffset += glm::clamp(targetFovOffset - speedFovOffset, -maxFovStep, maxFovStep);

        if (grounded)
            coyoteRemaining = coyoteDuration;
        else
            coyoteRemaining = std::max(0.0f, coyoteRemaining - dt);

        //-----------------------------------------------------------------------------
        // jump

        float downwardGravity = -physics.gravity.y * jumpGravityMultiplier;

        bool jumpLaunchedThisStep = false;

        // jump again
        if (!sliding && coyoteRemaining > 0.0f && jumpBufferRemaining > 0.0f) {

            float launchSpeed = std::sqrt(2.0f * downwardGravity * std::max(jumpHeight, 0.0f));

            physics.velocity.y = launchSpeed;

            glm::vec3 horizontalVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

            slideCarrySpeed = glm::length(horizontalVelocity);
            slideCarryDirection = slideCarrySpeed > 0.001f ? glm::normalize(horizontalVelocity) : slideVelocityDirection;

            float slideSpeedForJump = !shiftDown ? releasedSlideSpeed : 0.0f;
            glm::vec3 slideDirectionForJump = releasedSlideDirection;

            if (slideSpeedForJump > slideCarrySpeed) {
                slideCarrySpeed = slideSpeedForJump;
                slideCarryDirection = slideDirectionForJump;
            }

            slideJumpReleaseWindowRemaining = 0.0f;
            releasedSlideSpeed = 0.0f;
            releasedSlideDirection = glm::vec3(0.0f);

            slideCarryActive = slideCarrySpeed > 0.001f;

            sliding = false;

            jumpBufferRemaining = 0.0f;

            coyoteRemaining = 0.0f;

            grounded = false;
            jumpLaunchedThisStep = true;
        }

        if (slideCarryActive) {

            if (!grounded || jumpLaunchedThisStep) {

                slideCarryLandingGraceRemaining = 0.0f;
                slideCarryLandingGraceStarted = false;

            } else if (!sliding) {

                if (!slideCarryLandingGraceStarted) {

                    slideCarryLandingGraceRemaining = std::max(slideCarryLandingGraceDuration, 0.0f);
                    slideCarryLandingGraceStarted = true;

                } else
                    slideCarryLandingGraceRemaining = std::max(0.0f, slideCarryLandingGraceRemaining - dt);

                if (slideCarryLandingGraceRemaining <= 0.0f) {

                    slideCarryActive = false;

                    slideCarrySpeed = 0.0f;

                    slideCarryDirection = glm::vec3(0.0f);

                    slideCarryLandingGraceStarted = false;
                }
            }
        }

        if (shiftDown && !grounded && physics.velocity.y <= 0.0f)
            AddForce(player, glm::vec3(0.0f, -4000.0f, 0.0f));

        if (!grounded && physics.useGravity && jumpGravityMultiplier > 1.0f) {

            float extraGravityForce = -physics.gravity.y * (jumpGravityMultiplier - 1.0f) / physics.inverseMass;

            AddForce(player, glm::vec3(0.0f, -extraGravityForce, 0.0f));
        }

        if (glm::length2(moveDirection) > 0.0001f)
            moveDirection = glm::normalize(moveDirection);

        if (sliding && hasMove) {

            float maxTurn = glm::radians(60.0f) * dt;
            float dot = glm::clamp(glm::dot(slideVelocityDirection, moveDirection), -1.0f, 1.0f);

            float angle = std::atan2(slideVelocityDirection.z * moveDirection.x - slideVelocityDirection.x * moveDirection.z, dot);

            angle = glm::clamp(angle, -maxTurn, maxTurn);

            float c = std::cos(angle);
            float s = std::sin(angle);
            glm::vec3 oldDirection = slideVelocityDirection;

            slideVelocityDirection = glm::normalize(glm::vec3(c * oldDirection.x + s * oldDirection.z, 0.0f, -s * oldDirection.x + c * oldDirection.z));
        }

        if (sliding) {

            float turnFactor = 1.0f;

            if (hasMove) {

                float alignment = glm::clamp(glm::dot(slideDirection, slideVelocityDirection), -1.0f, 1.0f);

                float alignment01 = (alignment + 1.0f) * 0.5f;
                turnFactor = glm::mix(0.4f, 1.0f, alignment01);

                if (grounded)
                    slideSpeedCeiling = std::max(0.0f, slideSpeedCeiling - normalMaximumSpeed * 0.50f * dt);

                float targetSpeed = slideSpeedCeiling * turnFactor;

                float response = normalMaximumSpeed * 2.0f * dt;

                slideCurrentSpeed += glm::clamp(targetSpeed - slideCurrentSpeed, -response, response);

            } else if (grounded)
                slideCurrentSpeed = std::max(0.0f, slideCurrentSpeed - normalMaximumSpeed * 2.5f * dt);

            physics.maxHorizontalSpeed = normalMaximumSpeed * 2.f;

            if (dt > 0.0f) {

                float dragFactor = std::max(1.0f - physics.drag * dt, 0.01f);

                glm::vec3 currentVelocity(physics.velocity.x, 0.0f, physics.velocity.z);

                glm::vec3 targetVelocity = slideVelocityDirection * slideCurrentSpeed;

                glm::vec3 acceleration = (targetVelocity / dragFactor - currentVelocity) / dt;

                AddForce(player, acceleration / physics.inverseMass);
            }

        } else if (slideCarryActive && (!grounded || slideCarryLandingGraceRemaining > 0.0f)) {

            if (!grounded && hasMove && glm::length2(slideCarryDirection) > 0.0001f) {

                float maxTurn = glm::radians(slideAirTurnDegreesPerSecond) * dt;

                float dot = glm::clamp(glm::dot(slideCarryDirection, moveDirection), -1.0f, 1.0f);

                float angle = std::atan2(slideCarryDirection.z * moveDirection.x - slideCarryDirection.x * moveDirection.z, dot);
                angle = glm::clamp(angle, -maxTurn, maxTurn);

                float c = std::cos(angle);
                float s = std::sin(angle);

                glm::vec3 oldDirection = slideCarryDirection;

                slideCarryDirection = glm::normalize(glm::vec3(c * oldDirection.x + s * oldDirection.z, 0.0f, -s * oldDirection.x + c * oldDirection.z));
            }

            physics.maxHorizontalSpeed = std::max(normalMaximumSpeed, slideCarrySpeed);

            if (dt > 0.0f && slideCarrySpeed > 0.001f) {

                float dragFactor = std::max(1.0f - physics.drag * dt, 0.01f);

                glm::vec3 currentVelocity(physics.velocity.x, 0.0f, physics.velocity.z);
                glm::vec3 targetVelocity = slideCarryDirection * slideCarrySpeed;
                glm::vec3 acceleration = (targetVelocity / dragFactor - currentVelocity) / dt;

                AddForce(player, acceleration / physics.inverseMass);
            }

        } else {

            float crouchSpeedFactor = glm::mix(1.0f, crouchedMoveSpeedMultiplier, crouchAmount);
            physics.maxHorizontalSpeed = normalMaximumSpeed * crouchSpeedFactor;

            if (hasMove) {

                float drag = std::max(physics.drag, 1.0f);
                float dragFactor = std::max(1.0f - drag * dt, 0.01f);
                float driveAcceleration = physics.maxHorizontalSpeed * drag / dragFactor;

                glm::vec3 driveForce = moveDirection * (driveAcceleration / physics.inverseMass);

                AddForce(player, driveForce);
            }
        }
    }

    void UpdateCamera(Object &player, float dt) {

        constexpr float maxPitch = glm::radians(89.0f);

        float effectiveEyeHeight = cameraEyeHeight * (1.0f - (1.0f - slideCrouchScale) * crouchAmount);

        glm::vec3 desiredPosition = player.position + glm::vec3(0.0f, effectiveEyeHeight, 0.0f);

        if (!cameraInitialized) {

            glm::vec3 editorPosition = get_cam_pos();
            glm::vec3 editorLook = glm::normalize(cam.cam_target - editorPosition);

            cameraState.alpha = std::atan2(editorLook.z, editorLook.x);
            cameraState.beta = std::asin(glm::clamp(editorLook.y, -1.0f, 1.0f));
            cameraState.beta = glm::clamp(cameraState.beta, -maxPitch, maxPitch);
            cameraState.cam_position = desiredPosition;

            cameraInitialized = true;

        } else {

            constexpr float maxCameraLag = 0.12f;

            float playerSpeed = player.hasPhysics ? glm::length(player.physics.velocity) : 0.0f;
            float followSpeed = std::max(cameraFollowSpeed, playerSpeed / maxCameraLag);
            float followAmount = 1.0f - std::exp(-followSpeed * std::max(dt, 0.0f));

            cameraState.cam_position = glm::mix(cameraState.cam_position, desiredPosition, followAmount);
        }

        glm::vec3 lookDirection(std::cos(cameraState.alpha) * std::cos(cameraState.beta), std::sin(cameraState.beta), std::sin(cameraState.alpha) * std::cos(cameraState.beta));

        cameraState.cam_target = cameraState.cam_position + lookDirection;
        cameraState.cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

}
