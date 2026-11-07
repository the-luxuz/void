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
inline bool is_hdr = false;

// viewport
inline GLuint viewport_texture = 0;

inline int viewport_width = 512;
inline int viewport_height = 512;

// depth map
inline constexpr int ShadowMapResolution = 512;
inline GLuint depth_texture[MAX_LIGHTS]{};

// skybox
inline GLuint skybox_texture = 0;

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
GLuint LoadObjTextureFromMemory(const unsigned char *data, int size, bool sRGB = false);

//--------------------------------------------------------------------------
// Viewport and viewport depth texture

void Create_viewport_tex(int width, int height);
void CreateViewportDepthTexture(int width, int height);

void Create_depth_tex(int width, int height, int i);

//--------------------------------------------------------------------------
// Normal map

void LoadNormalMap(Object &obj);
