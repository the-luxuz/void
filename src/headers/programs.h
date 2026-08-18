#pragma once

#include "local.h"

//--------------------------------------------------------------------------
// global variables

// programs
extern GLuint program;
extern GLuint program_line;
extern GLuint program_grid;
extern GLuint program_depth;
extern GLuint program_skybox;
extern GLuint program_videoplayer;
extern GLuint program_hdrToCubemap;

//depth framebuffer
extern GLuint depthFBO[8];

//--------------------------------------------------------------------------
// Helper create shader

GLuint create_shaders(GLuint pro, const char* vertexShaderPath, const char* fragmentShaderPath);

//--------------------------------------------------------------------------
// init programs

void create_programs();