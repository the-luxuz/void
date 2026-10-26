#pragma once

#include "classes.h"

Object *FindObjectByID(EntityID id);

glm::mat4 GetLocalModelMatrix(const Object &obj);
glm::mat4 GetModelMatrix(const Object &obj);
void SetObjectWorldMatrix(Object &obj, const glm::mat4 &worldMatrix);

bool IsDescendantOf(EntityID objectID, EntityID ancestorID);
bool ReparentObject(Object &child, EntityID parentID);
void DetachChildren(EntityID parentID);

void GetWorldBounds(const Object &obj, glm::vec3 &min, glm::vec3 &max);
