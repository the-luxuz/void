#include "editor.h"

void Editor(){

    ImGui::Begin("ViewPort", nullptr);

    viewport_scroll_active = ImGui::IsWindowHovered();

    viewportPos = ImGui::GetCursorScreenPos();
    viewportSize = ImGui::GetContentRegionAvail();

    ImGui::Image((ImTextureID)(intptr_t)viewport_texture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    Guizmo();

    Browser();

    Hierarchy();

    Inspector();
    
    if (animationWindow) {

        animationEditor.animationSnapEnable = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT];

        UpdateAnimationImGui();
    }

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
