#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

#include <fstream>
#include <array>
#include <vector>

#include "local.h"
#include "classes.h"

struct Parser {
		
	bool load(const char* settings_name, const char* scene_name, Settings& st, camera& cam, std::vector<Object>& scene, std::vector<Object>& scene_lgh_obj);
	void save(const char* settings_name, const char* scene_name, Settings& st, camera& cam, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh);

	void load_settings(std::ifstream& in, Settings& st, camera& cam);
	void save_settings(std::ofstream& out, Settings& st, camera& cam);

	bool load_scene(std::ifstream& in, std::vector<Object>& scene_lgh_obj);
	void save_scene(std::ofstream& out, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh);

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
		GLuint obj_texture;

		GLuint normalTexture;
		std::string normal_map = "data/textures/brick_normal_map.png";
		bool has_normal_map = false;
		
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		glm::vec3 min, max;

		glm::mat4 model;
		glm::mat4 mvp;

		TextureMapping mapping = TextureMapping::UV;
		coord_types coordType = coord_types::XZ;
		cylinder_axis cyaxisType = cylinder_axis::Y;

		float shininess;

		Physics physics;
		bool hasPhysics = false;

		Collider collider;
		bool hasCollider = false;

		bool visible = true;
	};

	std::vector<Transform> objects;

	struct Light {

		std::string name;

		glm::vec3 pos;
		glm::vec3 dir;
		glm::vec3 col;
		glm::vec3 att;

		glm::quat rotation;

		float amb;
		float inner;
		float outer;
		float falloff;
		float bias;
		float pcf;

		std::string type = "POINT";
	};

	std::vector<Light> lights;

};