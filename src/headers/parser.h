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

	std::string win_name;
	int w, h;

	float fov;

	struct Transform {

		std::string mesh;
		
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

	};

	std::vector<Transform> objects;

};