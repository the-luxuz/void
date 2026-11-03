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

void init_model(const char *modelFile, Object &obj, bool normalizeImportedSize = false);
bool ImportModel(const std::string &path, std::vector<Object> &importedObjects,
                 std::vector<AnimationClip> *importedAnimations = nullptr, bool normalizeForEditor = true);
bool IsModelFormatSupported(const std::string &extension);

bool init_lights(Object& obj);

void init();

bool open_init(const char* settings, const char* scene, Settings& st);

//--------------------------------------------------------------------------
// main init function

void init_(const char* settings, const char* scene);
