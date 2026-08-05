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

#include "classes.h"

struct Parser {
		
	bool load(const char* fname, Settings& st);
	void save(const char* fname, Settings& st, const std::vector<Object>& scene);

	void WriteString(std::ofstream& out, const std::string& str);
	void ReadString(std::ifstream& in, std::string& str);

	template<typename T>

	void Write(std::ofstream& out, const T& t) {

		out.write(reinterpret_cast<const char*>(&t), sizeof(t));
	}

	template<typename M>

	void Read(std::ifstream& in, M& m) {

		in.read(reinterpret_cast<char*>(&m), sizeof(m));
	}

	std::string skybox;

	struct Transform {

		std::string obj_name;

		std::string mesh;

		std::string tex;
		
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		glm::vec3 min, max;

		TextureMapping mapping = TextureMapping::UV;
		coord_types coordType = coord_types::XZ;
		cylinder_axis cyaxisType = cylinder_axis::Y;

	};

	std::vector<Transform> objects;

};