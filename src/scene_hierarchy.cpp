#include "headers/scene_hierarchy.h"
#include "headers/helpers.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_set>

Object *FindObjectByID(EntityID id) {

    if (id == 0)
        return nullptr;

    for (Object &object : objects) {

        if (object.id == id)
            return &object;
    }

    return nullptr;
}

glm::mat4 GetLocalModelMatrix(const Object &obj) {

    glm::mat4 model(1.0f);

    model = glm::translate(model, obj.position);
    model *= glm::toMat4(obj.rotation);

    return glm::scale(model, obj.scale);
}

glm::mat4 GetModelMatrix(const Object &obj) {

    glm::mat4 model = GetLocalModelMatrix(obj);

    EntityID parentID = obj.parentID;

    for (size_t depth = 0; parentID != 0 && depth < objects.size(); depth++) {

        Object *parent = FindObjectByID(parentID);

        if (!parent)
            break;

        model = GetLocalModelMatrix(*parent) * model;

        parentID = parent->parentID;
    }

    return model;
}

void SetObjectWorldMatrix(Object &obj, const glm::mat4 &worldMatrix) {

    glm::mat4 localMatrix = worldMatrix;

    if (Object *parent = FindObjectByID(obj.parentID))
        localMatrix = glm::inverse(GetModelMatrix(*parent)) * worldMatrix;

    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(localMatrix, obj.scale, rotation, obj.position, skew, perspective); 

    obj.rotation = glm::normalize(rotation);
}

bool IsDescendantOf(EntityID objectID, EntityID ancestorID) {

    Object *current = FindObjectByID(objectID);

    for (size_t depth = 0; current && current->parentID != 0 && depth < objects.size(); depth++) {

        if (current->parentID == ancestorID)
            return true;

        current = FindObjectByID(current->parentID);
    }

    return false;
}

bool ReparentObject(Object &child, EntityID parentID) {

    if (child.parentID == parentID)
        return false;

    if (parentID == child.id || (parentID != 0 && !FindObjectByID(parentID)))
        return false;

    if (parentID != 0 && IsDescendantOf(parentID, child.id))
        return false;


    glm::mat4 worldMatrix = GetModelMatrix(child);

    child.parentID = parentID;

    SetObjectWorldMatrix(child, worldMatrix);

    return true;
}

void DetachChildren(EntityID parentID) {

    for (Object &object : objects) {

        if (object.parentID == parentID)
            ReparentObject(object, 0);
    }
}

void DeleteObjectHierarchy(EntityID rootID) {

    std::unordered_set<EntityID> hierarchy{rootID};
    bool addedChild = true;
    while (addedChild) {
        addedChild = false;
        for (const Object &object : objects) {
            if (hierarchy.contains(object.parentID) && hierarchy.insert(object.id).second)
                addedChild = true;
        }
    }

    for (size_t index = objects.size(); index-- > 0;) {
        if (!hierarchy.contains(objects[index].id))
            continue;

        DestroyBuffers(objects[index]);
        objects.erase(objects.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void GetWorldBounds(const Object &obj, glm::vec3 &min, glm::vec3 &max) {

    const glm::mat4 model = GetModelMatrix(obj);

    min = glm::vec3(FLT_MAX);
    max = glm::vec3(-FLT_MAX);

    for (int x = 0; x < 2; x++) {

        for (int y = 0; y < 2; y++) {

            for (int z = 0; z < 2; z++) {

                const glm::vec3 local(x ? obj.max.x : obj.min.x, y ? obj.max.y : obj.min.y, z ? obj.max.z : obj.min.z);
                const glm::vec3 world = glm::vec3(model * glm::vec4(local, 1.0f));

                min = glm::min(min, world);
                max = glm::max(max, world);
            }
        }
    }
}
