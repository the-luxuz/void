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

    const bool open = ImGui::TreeNodeEx(
        reinterpret_cast<void *>(static_cast<uintptr_t>(object.id)),
        flags,
        "%s",
        object.name.c_str());

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

        bool selected = (selectedObject == (int)(objects.size() + i));
        std::string label = lightObjects[i].name + "###light_" + std::to_string(lightObjects[i].id);

        if (ImGui::Selectable(label.c_str(), selected))
            selectedObject = objects.size() + i;
    }

    ImGui::End();
}
