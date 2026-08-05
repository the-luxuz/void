#pragma once

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "helpers.h"
#include "buffers.h"

extern SDL_Window* window;
extern SDL_GLContext context;

void init_viewport();

void init_grid();

void init_skybox(const char* skyboxFile, bool create_vaos);

void init_camera();

void init_obj(const char* objFile, Object& obj);

void init();

void init_(const char* settings);

void open_init(const char* settings, Settings& st);