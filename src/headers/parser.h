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
		
	bool load(const char* fname, Settings& st, camera& cam);
	void save(const char* fname, Settings& st, const std::vector<Object>& scene, camera& cam);

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

		std::string normal_map = "data/textures/default_normal.png";
		
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		float ns = 10.0f;

		glm::vec3 min, max;

		TextureMapping mapping = TextureMapping::UV;
		coord_types coordType = coord_types::XZ;
		cylinder_axis cyaxisType = cylinder_axis::Y;

	};

	std::vector<Transform> objects;

	struct Light {

		glm::vec3 pos;
		glm::vec3 dir;
		glm::vec3 col;
		glm::vec3 att;

		float amb = 0.0f;
		float inner = 0.0f;
		float outer = 30.0f;
		float falloff = 1.0f;
		float bias = 0.005f;
		float pcf = 1.0f;

		std::string type = "POINT";
	};

	std::vector<Light> lights;

};