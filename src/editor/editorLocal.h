#pragma once

#include "local.h"
#include "programs.h"
#include "helpers.h"
#include "inits.h"
#include "buffers.h"
#include "imgui_helpers.h"

inline ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
inline EntityID editingColliderID = 0;

// snap imguizmo
inline bool useSnap = false;
inline float *snap = nullptr;