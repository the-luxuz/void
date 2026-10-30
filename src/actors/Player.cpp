#include "Player.h"

#include "../headers/helpers.h"

namespace PlayerActor {

    void Control(const bool *keyboard, float dt, Object &player) {

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
