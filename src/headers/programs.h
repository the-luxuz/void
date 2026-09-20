#pragma once

#include "local.h"

//--------------------------------------------------------------------------
// global variables

// programs
inline GLuint program = 0;
inline GLuint program_line = 0;
inline GLuint program_grid = 0;
inline GLuint program_depth = 0;
inline GLuint program_skybox = 0;
inline GLuint program_videoplayer = 0;
inline GLuint program_hdrToCubemap = 0;

//depth framebuffer
inline GLuint depthFBO[MAX_LIGHTS]{};

//--------------------------------------------------------------------------
// Helper create shader

GLuint create_shaders(GLuint pro, const char* vertexShaderPath, const char* fragmentShaderPath);

//--------------------------------------------------------------------------
// init programs

void create_programs();
