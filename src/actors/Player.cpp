#include "Player.h"

#include "../headers/helpers.h"

namespace PlayerActor {

    void ResetJumpState() {

        jumpKeyWasDown = false;

        grounded = false;

        jumpBufferRemaining = 0.0f;

        jumpHoldRemaining = 0.0f;

        jumpHoldForceAcceleration = 0.0f;

        jumpReleaseBrakeRemaining = 0.0f;
    }

    void BeginCollisionStep() {

        grounded = false;
    }

    void RegisterCollision(const Object &a, const Object &b, const Contact &contact) {

        if (!contact.hit || a.collider.isTrigger || b.collider.isTrigger)
            return;

        glm::vec3 up(0.0f, 1.0f, 0.0f);

        if (a.name == "Player" && glm::dot(contact.normal, up) < -groundNormalThreshold) {
            grounded = true;
            jumpHoldRemaining = 0.0f;
            jumpHoldForceAcceleration = 0.0f;
            jumpReleaseBrakeRemaining = 0.0f;
        }

        if (b.name == "Player" && glm::dot(contact.normal, up) > groundNormalThreshold) {
            grounded = true;
            jumpHoldRemaining = 0.0f;
            jumpHoldForceAcceleration = 0.0f;
            jumpReleaseBrakeRemaining = 0.0f;
        }
    }

    void Control(const bool *keyboard, float dt, Object &player) {

        bool jumpKeyDown = keyboard[SDL_SCANCODE_SPACE];
        bool jumpReleased = !jumpKeyDown && jumpKeyWasDown;

        if (jumpKeyDown && !jumpKeyWasDown)
            jumpBufferRemaining = jumpBufferDuration;
        else if (jumpBufferRemaining > 0.0f)
            jumpBufferRemaining = std::max(0.0f, jumpBufferRemaining - dt);

        jumpKeyWasDown = jumpKeyDown;

        if (!player.hasPhysics || player.physics.isStatic)
            return;

        Physics &physics = player.physics;

        if (physics.inverseMass <= 0.0f)
            return;

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

        if (keyboard[SDL_SCANCODE_LSHIFT]) {
        
            if (!grounded)
                AddForce(player, glm::vec3(0.0f, -1500.f, 0.0f));

            defaultMaximumSpeed = 800;
            moveDirection *= 2;
            /*player.scale.y *= 0.5f;*/
        }else
            defaultMaximumSpeed = 450.0f;

        float downwardGravity = -physics.gravity.y * jumpGravityMultiplier;

        if (jumpReleased && jumpHoldRemaining > 0.0f && physics.velocity.y > 0.0f) {

            float remainingHoldFraction = jumpHoldDuration > 0.0f ? glm::clamp(jumpHoldRemaining / jumpHoldDuration, 0.0f, 1.0f) : 0.0f;

            jumpHoldRemaining = 0.0f;
            jumpHoldForceAcceleration = 0.0f;
            jumpReleaseBrakeRemaining = jumpReleaseBrakeDuration * remainingHoldFraction;
        }

        // jump again
        if (grounded && jumpBufferRemaining > 0.0f) {

            float minimumHeightRatio = glm::clamp(jumpMinimumHeightPercent * 0.01f, 0.05f, 0.95f);
            float minimumHeight = jumpHeight * minimumHeightRatio;
            float launchSpeed = std::sqrt(2.0f * downwardGravity * minimumHeight);

            physics.velocity.y = launchSpeed;

            jumpBufferRemaining = 0.0f;

            grounded = false;

            jumpReleaseBrakeRemaining = 0.0f;

            if (jumpKeyDown && physics.useGravity && launchSpeed > 0.0f && jumpHoldDuration > 0.0f) {
                jumpHoldRemaining = jumpHoldDuration;

                float holdTime = jumpHoldRemaining;
                float heightToGain = std::max(jumpHeight - minimumHeight, 0.0f);
                float quadraticTerm = 0.5f * downwardGravity * holdTime * holdTime;
                float linearTerm = launchSpeed * holdTime - quadraticTerm;
                float discriminant = linearTerm * linearTerm + 4.0f * quadraticTerm * heightToGain;
                float holdAccelerationMultiplier = (-linearTerm + std::sqrt(discriminant)) / (2.0f * quadraticTerm);

                jumpHoldForceAcceleration = downwardGravity * holdAccelerationMultiplier;
            } else {
                jumpHoldRemaining = 0.0f;
                jumpHoldForceAcceleration = 0.0f;
            }
        }

        if (!grounded && physics.useGravity && jumpGravityMultiplier > 1.0f) {

            float extraGravityForce = -physics.gravity.y * (jumpGravityMultiplier - 1.0f) / physics.inverseMass;

            AddForce(player, glm::vec3(0.0f, -extraGravityForce, 0.0f));
        }

        if (jumpHoldRemaining > 0.0f) {

            if (jumpKeyDown && physics.useGravity && physics.velocity.y > 0.0f) {

                float forceStep = std::min(dt, jumpHoldRemaining);
                float liftForce = jumpHoldForceAcceleration * (forceStep / dt) / physics.inverseMass;

                AddForce(player, glm::vec3(0.0f, liftForce, 0.0f));

                jumpHoldRemaining = std::max(jumpHoldRemaining - dt, 0.0f);
            } else {
                jumpHoldRemaining = 0.0f;
                jumpHoldForceAcceleration = 0.0f;
            }

            if (jumpHoldRemaining <= 0.0f)
                jumpHoldForceAcceleration = 0.0f;
        }

        if (jumpReleaseBrakeRemaining > 0.0f) {

            if (physics.useGravity && physics.velocity.y > 0.0f) {

                float forceStep = std::min(dt, jumpReleaseBrakeRemaining);
                float brakeAcceleration = -physics.gravity.y * jumpReleaseBrakeMultiplier;
                float brakeForce = -brakeAcceleration * (forceStep / dt) / physics.inverseMass;

                AddForce(player, glm::vec3(0.0f, brakeForce, 0.0f));

                jumpReleaseBrakeRemaining = std::max(jumpReleaseBrakeRemaining - dt, 0.0f);

            } else
                jumpReleaseBrakeRemaining = 0.0f;
        }

        if (glm::length2(moveDirection) > 0.0001f)
            moveDirection = glm::normalize(moveDirection);

        if (glm::length2(moveDirection) > 0.0001f) {

            float drag = std::max(physics.drag, 1.0f);

            float dragFactor = std::max(1.0f - drag * dt, 0.01f);

            float driveAcceleration = physics.maxHorizontalSpeed * drag / dragFactor;

            glm::vec3 driveForce = moveDirection * (driveAcceleration / physics.inverseMass);

            AddForce(player, driveForce);
        }
    }

    void UpdateCamera(Object &player, float dt) {

        constexpr float maxPitch = glm::radians(89.0f);
        glm::vec3 desiredPosition = player.position + glm::vec3(0.0f, cameraEyeHeight, 0.0f);

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
