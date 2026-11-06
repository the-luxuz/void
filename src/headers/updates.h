#pragma once

#include "local.h"
#include "programs.h"
#include "helpers.h"
#include "inits.h"
#include "buffers.h"
#include "imgui_helpers.h"

//--------------------------------------------------------------------------
// global variables

// current mesh, texture and operation for ImGuizmo
inline int currentMesh = 0;
inline GLuint currentTexture = 0;

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
// animations

void UpdateAnimations(float dt);

void UpdateAnimationPreview(float dt);

void UpdateAnimation(float dt);

//--------------------------------------------------------------------------
// Update Imgui and guizmo

void update_imgui_guizmo();

void DrawVideo(const VideoPlayer& video);
