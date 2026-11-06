#include "editor.h"

void SettingsWindow() {

    ImGui::Begin("Graphics Settings", &showGraphicsSettings);

    ImGui::Checkbox("VSync", &st.VSync);

    if (!st.VSync)
        SDL_GL_SetSwapInterval(0);
    else
        SDL_GL_SetSwapInterval(1);

    ImGui::Separator();

    ImGui::Checkbox("Show Light", &st.light_active);

    ImGui::Checkbox("Show Shadows", &st.shadow_active);

    ImGui::Checkbox("Show Normal Map", &st.normal_map_active);

    ImGui::Checkbox("Show Wireframe", &st.wireframe_active);

    ImGui::Checkbox("Active global Physics", &st.physics_active);

    ImGui::Checkbox("Active global Collisions", &st.collision_active);

    if (ImGui::SliderFloat("FOV", &st.fov, 30.0f, 120.0f, "%.1f°")) {

        projection = glm::perspective(glm::radians(st.fov), (float)viewport_width / viewport_height, cam.nearPlane,
                                      cam.farPlane);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Player Camera");
    ImGui::SliderFloat("Eye height", &PlayerActor::cameraEyeHeight, 0.5f, 3.0f, "%.2f");
    ImGui::SliderFloat("Follow response", &PlayerActor::cameraFollowSpeed, 1.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Look sensitivity", &PlayerActor::lookSensitivity, 0.0001f, 0.003f, "%.4f");

    ImGui::Separator();
    ImGui::TextUnformatted("Player Movement");
    ImGui::SliderFloat("Default max speed", &PlayerActor::defaultMaximumSpeed, 100.0f, 10000.0f, "%.0f");
    ImGui::SliderFloat("Slide crouch scale", &PlayerActor::slideCrouchScale, 0.4f, 1.0f, "%.2f");
    ImGui::SliderFloat("Crouch transition", &PlayerActor::crouchTransitionDuration, 0.05f, 0.5f, "%.2f s");
    ImGui::SliderFloat("Crouched speed multiplier", &PlayerActor::crouchedMoveSpeedMultiplier, 0.2f, 1.0f, "%.2f x");
    ImGui::SliderFloat("Air slide steering", &PlayerActor::slideAirTurnDegreesPerSecond, 0.0f, 540.0f, "%.0f deg/s");
    ImGui::SliderFloat("Landing inertia decay", &PlayerActor::slideCarryLandingDecayDuration, 0.1f, 0.8f, "%.2f s");
    ImGui::SliderFloat("Slide release jump window", &PlayerActor::slideJumpReleaseWindowDuration, 0.0f, 0.3f, "%.2f s");
    ImGui::SliderFloat("Speed FOV increase", &PlayerActor::speedFovIncrease, 0.0f, 12.0f, "%.1f°");
    ImGui::SliderFloat("FOV reference speed", &PlayerActor::speedFovReferenceMultiplier, 0.5f, 4.0f, "%.2f x normal");
    ImGui::SliderFloat("Jump height", &PlayerActor::jumpHeight, 20.0f, 600.0f, "%.0f");
    ImGui::SliderFloat("Air gravity multiplier", &PlayerActor::jumpGravityMultiplier, 1.0f, 5.0f, "%.1f x");

    ImGui::End();
}

void Editor(){

    ImGui::Begin("ViewPort", nullptr);

    viewport_scroll_active = ImGui::IsWindowHovered();

    viewportPos = ImGui::GetCursorScreenPos();
    viewportSize = ImGui::GetContentRegionAvail();

    ImGui::Image((ImTextureID)(intptr_t)viewport_texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    Browser();

    if (animationWindow) {

        animationEditor.animationSnapEnable = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT];

        UpdateAnimationImGui();
    }

    Guizmo();

    Hierarchy();

    if (showGraphicsSettings)
        SettingsWindow();

    Inspector();

    switch (currentTool) {

    case Tools::TRANSLATE:
        currentOperation = ImGuizmo::TRANSLATE;
        break;

    case Tools::ROTATE:
        currentOperation = ImGuizmo::ROTATE;
        break;

    case Tools::SCALE:
        currentOperation = ImGuizmo::SCALE;
        break;
    }
}
