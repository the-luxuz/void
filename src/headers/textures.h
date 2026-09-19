#pragma once

#include "classes.h"
#include "local.h"
#include "stb_image.h"
#include "tinyexr.h"

#include <filesystem>
#include <iostream>

//--------------------------------------------------------------------------
// global variables

// hdr check flag
extern bool is_hdr;

// viewport
extern GLuint viewport_texture;

extern int viewport_width;
extern int viewport_height;

// depth map
extern GLuint depth_texture[MAX_LIGHTS];

// skybox
extern GLuint skybox_texture;

//--------------------------------------------------------------------------
// EXR and HDR texture

GLuint LoadEXRTexture(const std::string &path);

GLuint LoadHDRTexture(const std::string &path);

//--------------------------------------------------------------------------
// Skybox

void Load_Skybox(const std::string &path);

//--------------------------------------------------------------------------
// OBJ texture

GLuint LoadObjTexture(const std::string &path, bool sRGB = false);

//--------------------------------------------------------------------------
// Viewport and viewport depth texture

void Create_viewport_tex(int width, int height);
void CreateViewportDepthTexture(int width, int height);

void Create_depth_tex(int width, int height, int i);

//--------------------------------------------------------------------------
// Normal map

void LoadNormalMap(Object &obj);
