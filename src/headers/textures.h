#pragma once

#include "stb_image.h"

#include "tinyexr.h"

#include "local.h"
#include "classes.h"

#include <filesystem>
#include <iostream>

extern bool is_hdr;

extern int viewport_width;
extern int viewport_height;

extern GLuint viewport_texture;
extern GLuint depth_texture[8];

extern GLuint skybox_texture;

GLuint LoadEXRTexture(const std::string& path);

GLuint LoadHDRTexture(const std::string& path);

void Load_Skybox(const std::string& path);

GLuint LoadObjTexture(const std::string& path);

void Create_viewport_tex(int width, int height);

void Create_depth_tex(int width, int height, int i);

void LoadNormalMap(Object& obj);