#pragma once

#include "local.h"

extern GLuint program;
extern GLuint program_line;
extern GLuint program_grid;
extern GLuint program_depth;
extern GLuint program_skybox;
extern GLuint program_hdrToCubemap;

extern GLuint depthFBO[8];

GLuint create_shaders(GLuint pro, const char* vertexShaderPath, const char* fragmentShaderPath);

void create_programs();