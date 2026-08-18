#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imguizmo.h"

#include <vector>
#include <string>

#include <iostream>
#include <sstream>
#include <fstream>

#include "tiny_obj_loader.h"

#include <nfd.h>

#include <glm/gtx/quaternion.hpp>

extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

}

const int MAX_LIGHTS = 8;