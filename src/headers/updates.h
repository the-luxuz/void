#pragma once

#include "local.h"
#include "programs.h"
#include "helpers.h"
#include "inits.h"
#include "buffers.h"

extern int currentMesh;

extern GLuint currentTexture;

extern bool useSnap;

extern float* snap;

extern glm::mat4 m;

extern const float cellSize;

extern ObjectState prevState;

extern ImGuizmo::OPERATION currentOperation;

void update_grid(glm::vec3 cameraPos);

void update_objects();

void update_skybox();

void update_imgui_guizmo();