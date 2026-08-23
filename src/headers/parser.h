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
		
	bool load(const char* settings_name, const char* scene_name, Settings& st, camera& cam, std::vector<Object>& scene);
	void save(const char* settings_name, const char* scene_name, Settings& st, camera& cam, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh);

	void load_settings(std::ifstream& in, Settings& st, camera& cam);
	void save_settings(std::ofstream& out, Settings& st, camera& cam);

	bool load_scene(std::ifstream& in);
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

	void WriteAnimationValue(std::ofstream& out, const AnimationValue& value);

	void ReadAnimationValue(std::ifstream& in, AnimationValue& value);

	void WriteAnimationKey(std::ofstream& out, const AnimationKey& key);

	void ReadAnimationKey(std::ifstream& in, AnimationKey& key);

	void WriteAnimationTrack(std::ofstream& out, const AnimationTrack& track);

	void ReadAnimationTrack(std::ifstream& in, AnimationTrack& track);

	void WriteAnimationClip(std::ofstream& out, const AnimationClip& clip);

	void ReadAnimationClip(std::ifstream& in, AnimationClip& clip);

	std::string skybox;

	struct Transform {

		EntityID id;

		std::string obj_name;

		std::string mesh;

		std::string tex;

		std::string normal_map = "data/textures/brick_normal_map.png";
		bool hasNormalMap = false;
		
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		glm::vec3 min, max;

		TextureMapping mapping = TextureMapping::UV;
		coord_types coordType = coord_types::XZ;
		cylinder_axis cyaxisType = cylinder_axis::Y;

		float shininess;

		Physics physics;
		bool hasPhysics = false;

		Collider collider;
		bool hasCollider = false;

		bool hasMaterial = true;

		bool hasMapping = true;

		bool hasAnimation = true;

		bool tiling = false;

		bool visible = true;
	};

	std::vector<Transform> objects;

	struct Light {

		EntityID id;

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

		bool hasAnimation = true;

	};

	std::vector<Light> lights;
	std::vector<Transform> lights_objects;

	std::vector<AnimationClip> anim;
	std::vector<AnimationPlayer> animPlayer;
	AnimationEditorState animEditor;

};
