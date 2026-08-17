#pragma once

#include "local.h"
#include "classes.h"
#include "helpers.h"

void UpdateAnimationImGui();

void DrawObjectAnimationButtons(Object& obj);

void DrawLightAnimationButtons(Light& light);

void DrawAnimationTimeline(AnimationClip& clip);