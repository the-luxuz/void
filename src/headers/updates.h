#pragma once

#include "local.h"
#include "programs.h"
#include "helpers.h"
#include "inits.h"
#include "buffers.h"

//--------------------------------------------------------------------------
// global variables

// current mesh, texture and operation for ImGuizmo
extern int currentMesh;
extern GLuint currentTexture;
extern ImGuizmo::OPERATION currentOperation;

// snap imguizmo
extern bool useSnap;
extern float* snap;

//--------------------------------------------------------------------------
// Update Grid

void update_grid(glm::vec3 cameraPos);

//--------------------------------------------------------------------------
// Update Depth Map

void update_depthmap();

//--------------------------------------------------------------------------
// Update Objects

void update_objects();

//--------------------------------------------------------------------------
// Update Skybox

void update_skybox();

//--------------------------------------------------------------------------
// Update Physics

void PhysicsUpdate(float dt);

//--------------------------------------------------------------------------
// Update Collisions

void CollisionUpdate();

//--------------------------------------------------------------------------
// Update Imgui and guizmo

void update_imgui_guizmo();