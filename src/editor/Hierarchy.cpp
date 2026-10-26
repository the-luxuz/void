#include "Hierarchy.h"

static int FindObjectIndex(EntityID id) {

    for (size_t index = 0; index < objects.size(); ++index) {

        if (objects[index].id == id)
            return static_cast<int>(index);
    }

    return -1;
}

static void DrawHierarchyNode(Object &object, size_t depth = 0) {

    if (depth >= objects.size())
        return;

    bool hasChildren = false;

    for (const Object &candidate : objects) {

        if (candidate.parentID == object.id) {

            hasChildren = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selectedObject == FindObjectIndex(object.id))
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(object.id)));

    const bool open = ImGui::TreeNodeEx(object.name.c_str(), flags);

    if (ImGui::IsItemClicked())
        selectedObject = FindObjectIndex(object.id);

    if (ImGui::BeginDragDropSource()) {

        ImGui::SetDragDropPayload("HIERARCHY_OBJECT", &object.id, sizeof(object.id));

        ImGui::TextUnformatted(object.name.c_str());

        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {

        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT")) {

            EntityID childID = 0;

            memcpy(&childID, payload->Data, sizeof(childID));

            if (Object *child = FindObjectByID(childID))
                ReparentObject(*child, object.id);
        }

        ImGui::EndDragDropTarget();
    }

    if (hasChildren && open) {

        for (Object &child : objects) {

            if (child.parentID == object.id)
                DrawHierarchyNode(child, depth + 1);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}


void Hierarchy(){

    //----------------------------------------------------
    // hierarchy

    ImGuiWindowFlags h_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::Begin("Hierarchy", nullptr, h_flags);

    ImGui::Selectable("Scene Root", false);
    if (ImGui::BeginDragDropTarget()) {

        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT")) {

            EntityID childID = 0;

            memcpy(&childID, payload->Data, sizeof(childID));

            if (Object *child = FindObjectByID(childID))
                ReparentObject(*child, 0);
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::Button("+ Empty")) {

        objects.push_back(CreateEmptyObject());
        selectedObject = static_cast<int>(objects.size()) - 1;
    }

    ImGui::Separator();

    for (Object &object : objects) {

        if (object.parentID == 0 || !FindObjectByID(object.parentID))
            DrawHierarchyNode(object);
    }

    if (!lightObjects.empty())
        ImGui::SeparatorText("Lights");

    for (size_t i = 0; i < lightObjects.size(); i++) {

        ImGui::PushID((int)(objects.size() + i));

        bool selected = (selectedObject == (int)(objects.size() + i));

        if (ImGui::Selectable(lightObjects[i].name.c_str(), selected))
            selectedObject = objects.size() + i;

        ImGui::PopID();
    }

    ImGui::End();
}