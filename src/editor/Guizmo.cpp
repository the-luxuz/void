#include "Guizmo.h"


//--------------------------------------------------------------------------
// Update Collisions

Object GetColliderProxy(const Object &object) {

    glm::mat4 transform = GetModelMatrix(object);

    transform = glm::translate(transform, object.colliderTransform.offset);
    transform *= glm::toMat4(object.colliderTransform.rotation);
    transform = glm::scale(transform, object.colliderTransform.size);

    glm::vec3 position(0.0f);
    glm::vec3 rotation(0.0f);
    glm::vec3 scale(1.0f);

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), glm::value_ptr(position), glm::value_ptr(rotation), glm::value_ptr(scale));

    Object proxy;

    proxy.position = position;
    proxy.rotation = glm::quat(glm::radians(rotation));
    proxy.scale = scale;
    proxy.collider = object.collider;

    return proxy;
}


static void DrawColliderWireframe(Object &object) {

    if (!object.hasCollider || object.collider.type == ColliderType::None)
        return;

    Object proxy = GetColliderProxy(object);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), proxy.position) * glm::toMat4(proxy.rotation);

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    ImU32 color = IM_COL32(80, 230, 120, 255);

    glm::mat4 viewProjection = projection * view;

    auto project = [&](glm::vec3 &local, ImVec2 &screen) {
        glm::vec4 clip = viewProjection * transform * glm::vec4(local, 1.0f);

        if (clip.w <= 0.001f)
            return false;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        screen.x = viewportPos.x + (ndc.x * 0.5f + 0.5f) * viewportSize.x;
        screen.y = viewportPos.y + (0.5f - ndc.y * 0.5f) * viewportSize.y;

        return true;
    };

    auto addLine = [&](glm::vec3 &a, glm::vec3 &b) {
        ImVec2 screenA;
        ImVec2 screenB;

        if (project(a, screenA) && project(b, screenB))
            drawList->AddLine(screenA, screenB, color, 2.0f);
    };

    drawList->PushClipRect(viewportPos, ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y), true);

    if (object.collider.type == ColliderType::Sphere) {

        float radius = object.collider.radius * glm::compMax(glm::abs(proxy.scale));

        int segments = 32;

        for (int plane = 0; plane < 3; ++plane) {

            for (int i = 0; i < segments; ++i) {

                float a = glm::two_pi<float>() * static_cast<float>(i) / segments;
                float b = glm::two_pi<float>() * static_cast<float>(i + 1) / segments;

                glm::vec3 pointA;
                glm::vec3 pointB;

                if (plane == 0) {

                    pointA = glm::vec3(0.0f, std::cos(a), std::sin(a)) * radius;
                    pointB = glm::vec3(0.0f, std::cos(b), std::sin(b)) * radius;

                } else if (plane == 1) {

                    pointA = glm::vec3(std::cos(a), 0.0f, std::sin(a)) * radius;
                    pointB = glm::vec3(std::cos(b), 0.0f, std::sin(b)) * radius;

                } else {

                    pointA = glm::vec3(std::cos(a), std::sin(a), 0.0f) * radius;
                    pointB = glm::vec3(std::cos(b), std::sin(b), 0.0f) * radius;
                }

                addLine(pointA, pointB);
            }
        }
    } else {

        float x = proxy.scale.x;
        float y = object.collider.type == ColliderType::Plane ? 0.0f : proxy.scale.y;
        float z = proxy.scale.z;

        glm::vec3 corners[8] = {{-x, -y, -z}, {x, -y, -z}, {-x, y, -z}, {x, y, -z},
                                {-x, -y, z},  {x, -y, z},  {-x, y, z},  {x, y, z}};

        int edges[12][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {4, 5}, {4, 6},
                            {5, 7}, {6, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto &edge : edges)
            addLine(corners[edge[0]], corners[edge[1]]);
    }

    drawList->PopClipRect();
}

void Guizmo(){

    Object *obj = GetSelectedObject();
    Light *light = GetSelectedLight();

    if (obj) {

        ImGuizmo::BeginFrame();

        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

        ImGuizmo::SetOrthographic(false);

        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

        DrawColliderWireframe(*obj);

        float moveSnap[3] = {12.5, 12.5, 12.5};
        float scaleSnap[3] = {0.125, 0.125, 0.125};
        float rotateSnap[3] = {11.25, 11.25, 11.25};

        snap = nullptr;

        useSnap = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT];

        if (useSnap) {

            switch (currentOperation) {

            case ImGuizmo::TRANSLATE:
                snap = moveSnap;
                break;

            case ImGuizmo::ROTATE:
                snap = rotateSnap;
                break;

            case ImGuizmo::SCALE:
                snap = scaleSnap;
                break;
            }
        }

        const bool editingCollider = editingColliderID == obj->id && obj->hasCollider && obj->collider.type != ColliderType::None;

        obj->model = GetModelMatrix(*obj);

        glm::mat4 gizmoModel = obj->model;

        if (editingCollider) {

            gizmoModel = glm::translate(gizmoModel, obj->colliderTransform.offset);
            gizmoModel *= glm::toMat4(obj->colliderTransform.rotation);
            gizmoModel = glm::scale(gizmoModel, obj->colliderTransform.size);
        }

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentOperation, ImGuizmo::WORLD, glm::value_ptr(gizmoModel), nullptr, snap);

        static bool wasUsing = false;

        bool usingGizmo = ImGuizmo::IsUsing();

        if (usingGizmo && !wasUsing)
            prevState = CaptureState(*obj);

        if (usingGizmo && !wasUsing && light)
            prevLightState = CaptureLightState(*light);

        if (ImGuizmo::IsUsing()) {

            if (editingCollider) {

                glm::mat4 localCollider = glm::inverse(GetModelMatrix(*obj)) * gizmoModel;
                glm::vec3 translation(0.0f);
                glm::vec3 scale(1.0f);
                glm::vec3 euler(0.0f);

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localCollider), glm::value_ptr(translation), glm::value_ptr(euler), glm::value_ptr(scale));

                if (useSnap && currentOperation == ImGuizmo::ROTATE) {
                    auto Snap = [](float value, float step) { return roundf(value / step) * step; };
                    euler.x = Snap(euler.x, rotateSnap[0]);
                    euler.y = Snap(euler.y, rotateSnap[1]);
                    euler.z = Snap(euler.z, rotateSnap[2]);
                }

                obj->colliderTransform.offset = translation;
                obj->colliderTransform.rotation = glm::quat(glm::radians(euler));
                obj->colliderTransform.size = glm::max(glm::abs(scale), glm::vec3(0.01f));

            } else {

                glm::vec3 translation = glm::vec3(0.0f);
                glm::vec3 scale = glm::vec3(20.0f);
                glm::vec3 euler;

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(gizmoModel), glm::value_ptr(translation), glm::value_ptr(euler), glm::value_ptr(scale));

                if (useSnap && currentOperation == ImGuizmo::ROTATE) {

                    auto Snap = [](float value, float step) { return roundf(value / step) * step; };

                    euler.x = Snap(euler.x, rotateSnap[0]);
                    euler.y = Snap(euler.y, rotateSnap[1]);
                    euler.z = Snap(euler.z, rotateSnap[2]);
                }

                if (useSnap && currentOperation == ImGuizmo::ROTATE) {

                    gizmoModel = glm::translate(glm::mat4(1.0f), translation);
                    gizmoModel *= glm::toMat4(glm::quat(glm::radians(euler)));
                    gizmoModel = glm::scale(gizmoModel, scale);
                }

                SetObjectWorldMatrix(*obj, gizmoModel);
            }
        }

        if (!usingGizmo && wasUsing) {

            obj->undoStack.push_back(prevState);
            obj->redoStack.clear();
        }

        if (!usingGizmo && wasUsing && light) {

            light->undoStack.push_back(prevLightState);
            light->redoStack.clear();
        }

        wasUsing = usingGizmo;

        if (currentOperation != ImGuizmo::SCALE && light)
            obj->scale = glm::vec3(20.0f);
    }

    ImGui::End();
}