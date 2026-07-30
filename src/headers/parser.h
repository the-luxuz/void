#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

#include <fstream>
#include <array>
#include <vector>

struct Parser {
		
	void load(const char* fname);

	enum  class window_mode {
		WINDOWED,
		WINDOWED_BORDERLESS,
		FULLSCREEN
	};

	window_mode win_mode;

	window_mode last_win_mode;

	std::string win_name;
	int w, h;

	float fov;
	std::string skybox;

	struct Transform {

		std::string obj_name;

		std::string mesh;

		std::string tex;
		
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

	};

	std::vector<Transform> objects;

};