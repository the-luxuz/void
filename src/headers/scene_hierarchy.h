#pragma once

#include "classes.h"

// Scene graph helpers. Object transforms are stored locally and this module
// resolves their world-space representation for rendering, picking and tools.
Object *FindObjectByID(EntityID id);

glm::mat4 GetLocalModelMatrix(const Object &obj);
glm::mat4 GetModelMatrix(const Object &obj);
void SetObjectWorldMatrix(Object &obj, const glm::mat4 &worldMatrix);

bool IsDescendantOf(EntityID objectID, EntityID ancestorID);
bool ReparentObject(Object &child, EntityID parentID);
void DetachChildren(EntityID parentID);

void GetWorldBounds(const Object &obj, glm::vec3 &min, glm::vec3 &max);
