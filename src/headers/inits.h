#pragma once

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "helpers.h"
#include "buffers.h"

//--------------------------------------------------------------------------
// global variables

// window and context
inline SDL_Window* window = nullptr;
inline SDL_GLContext context = nullptr;

inline Shader_Uniform shader_uniform{};

//--------------------------------------------------------------------------
// init functions

void init_uniforms();

void init_class_uniforms();

void init_viewport();

void init_grid();

void init_skybox(const char* skyboxFile, bool create_vaos);

void init_camera();

void init_obj(const char* objFile, Object& obj);
bool ImportGltf(const std::string& path, std::vector<Object>& importedObjects);
void PrepareImportedObject(Object& obj);

bool init_lights(Object& obj);

void init();

bool open_init(const char* settings, const char* scene, Settings& st);

//--------------------------------------------------------------------------
// main init function

void init_(const char* settings, const char* scene);
