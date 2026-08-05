#pragma once

#include "local.h"

extern GLuint program;
extern GLuint program_grid;
extern GLuint program_skybox;
extern GLuint program_hdrToCubemap;

GLuint create_shaders(GLuint pro, const char* vertexShaderPath, const char* fragmentShaderPath);

void create_programs();