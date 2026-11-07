#include "editor.h"

void SettingsWindow() {

    ImGui::Begin("Graphics Settings", &showGraphicsSettings);

    if (ImGui::Checkbox("VSync", &st.VSync))
        SDL_GL_SetSwapInterval(st.VSync ? 1 : 0);

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

    GetGameAPI()->DrawSettingsUI();

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
