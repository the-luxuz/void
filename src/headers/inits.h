#pragma once

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "helpers.h"
#include "buffers.h"

//--------------------------------------------------------------------------
// global variables

// window and context
extern SDL_Window* window;
extern SDL_GLContext context;

//--------------------------------------------------------------------------
// init functions

void init_viewport();

void init_grid();

void init_skybox(const char* skyboxFile, bool create_vaos);

void init_camera();

void init_obj(const char* objFile, Object& obj);

void init_obj(const char* objFile, Light& obj);

bool init_lights(Object& obj);

void init();

bool open_init(const char* settings, const char* scene, Settings& st);

//--------------------------------------------------------------------------
// main init function

void init_(const char* settings, const char* scene);
