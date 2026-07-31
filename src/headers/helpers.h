#pragma once

#include "local.h"
#include "classes.h"
#include "programs.h"
#include "textures.h"

GLuint cubeFBO;
GLuint dynamic_cubemap;

GLuint captureFBO;
GLuint captureRBO;

camera cam;
Parser par;

float alpha, beta, radius;

const float STEP = 25.0f;

GLuint gridVAO;
GLuint gridVBO;

int gridVertexCount;

float moveSpeed = 10.0f;
float mouseSensitivity = 0.005f;

int selectedObject = -1;

GLuint skyVAO;
GLuint skyVBO;

glm::mat4 view;
glm::mat4 projection;

glm::vec3 rayOrigin;
glm::vec3 rayDir;

std::vector<Object> objects;

GLuint skyboxVAO;
GLuint skyboxVBO;

glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

glm::mat4 captureViews[] = {

    glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1,0))
};

std::vector<std::filesystem::path> pathTextures;

std::string dirPath = "data/textures";

std::filesystem::path rootFolder = "../data";
std::filesystem::path currentFolder = rootFolder;

std::unordered_map<std::string, int> objectNameCounters;

bool reloadFolder = false;
std::filesystem::path newFolder;

std::vector<std::filesystem::directory_entry> folderEntries;

void FixCylinderSeam(Object& obj) {

	for (size_t i = 0; i < obj.indices.size(); i += 3) {

		Vertex& v0 = obj.vertices[obj.indices[i + 0]];
		Vertex& v1 = obj.vertices[obj.indices[i + 1]];
		Vertex& v2 = obj.vertices[obj.indices[i + 2]];

		float u0 = v0.uv.x;
		float u1 = v1.uv.x;
		float u2 = v2.uv.x;

		float minU = std::min({ u0, u1, u2 });
		float maxU = std::max({ u0, u1, u2 });

		if (maxU - minU > 0.5f) {

			if (u0 < 0.5f) v0.uv.x += 1.0f;
			if (u1 < 0.5f) v1.uv.x += 1.0f;
			if (u2 < 0.5f) v2.uv.x += 1.0f;
		}
	}
}

void RestoreOriginalUV(Object& obj) {

	for(auto& vertex : obj.vertices) {
		vertex.uv = vertex.originalUV;
	}
}

void GeneratePlanarUV(Object& obj, coord_types coordType) {

	switch (coordType) {

		case coord_types::XZ: {

			float sizeX = obj.max.x - obj.min.x;
			float sizeZ = obj.max.z - obj.min.z;

			if (sizeX == 0.0f) sizeX = 1.0f;
			if (sizeZ == 0.0f) sizeZ = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.x - obj.min.x) / sizeX;
				vertex.uv.y = (vertex.position.z - obj.min.z) / sizeZ;
			}
			break;
		}

		case coord_types::XY: {

			float sizeX = obj.max.x - obj.min.x;
			float sizeY = obj.max.y - obj.min.y;

			if (sizeX == 0.0f) sizeX = 1.0f;
			if (sizeY == 0.0f) sizeY = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.x - obj.min.x) / sizeX;
				vertex.uv.y = (vertex.position.y - obj.min.y) / sizeY;
			}
			break;
		}

		case coord_types::YZ: {

			float sizeY = obj.max.y - obj.min.y;
			float sizeZ = obj.max.z - obj.min.z;

			if (sizeY == 0.0f) sizeY = 1.0f;
			if (sizeZ == 0.0f) sizeZ = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.y - obj.min.y) / sizeY;
				vertex.uv.y = (vertex.position.z - obj.min.z) / sizeZ;
			}
			break;
		}
	}
}

void GenerateCubeUV(Object& obj) {


}

void GenerateCylindricalUV(Object& obj, cylinder_axis axis) {

	static float doble_pi = 2.0f * glm::pi<float>();

	switch (axis) {
	
		case cylinder_axis::X: {
	
			float size = obj.max.x - obj.min.x;
			if (size == 0.0f) 
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.y, vertex.position.z);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.x - obj.min.x) / size;
			}

			break;
		}

		case cylinder_axis::Y: {

			float size = obj.max.y - obj.min.y;
			if (size == 0.0f) 
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.z, vertex.position.x);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.y - obj.min.y) / size;
			}

			break;
		}

		case cylinder_axis::Z: {

			float size = obj.max.z - obj.min.z;
			if (size == 0.0f)
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.y, vertex.position.x);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.z - obj.min.z) / size;
			}

			break;
		}
	}

	FixCylinderSeam(obj);
}

void GenerateSphericalUV(Object& obj) {


}

void UpdateMapping(Object& obj) {

	switch (obj.mapping) {

	case TextureMapping::UV:
		RestoreOriginalUV(obj);
		break;

	case TextureMapping::Planar:
		GeneratePlanarUV(obj, obj.coordType);
		break;

	case TextureMapping::Cubic:
		GenerateCubeUV(obj);
		break;

	case TextureMapping::Cylindrical:
		GenerateCylindricalUV(obj, obj.cyaxisType);
		break;

	case TextureMapping::Spherical:
		GenerateSphericalUV(obj);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferSubData(GL_ARRAY_BUFFER,
		0,
		obj.vertices.size() * sizeof(Vertex),
		obj.vertices.data());
}

void ReloadFolder() {

	folderEntries.clear();

	for (auto& e : std::filesystem::directory_iterator(currentFolder))
		folderEntries.push_back(e);
}

std::string GenerateObjectName(const std::string& baseName) {

	int& counter = objectNameCounters[baseName];

	if (counter == 0) {

		counter = 1;
		return baseName;
	}

	return baseName + "_" + std::to_string(counter++);
}

void read_textures_path() {

	pathTextures.clear();

	for (const auto& entry : std::filesystem::directory_iterator("../" + dirPath)) {

		if (!entry.is_regular_file())
			continue;

		pathTextures.push_back(entry.path());
	}
}

GLuint TextureManager::Get(const std::string& path) {

	auto it = textures.find(path);

	if (it != textures.end())
		return it->second;

	GLuint texture = LoadObjTexture(path);

	if (texture != 0)
		textures[path] = texture;

	return texture;
}

Object* GetSelectedObject() {

	if (selectedObject < 0)
		return nullptr;

	if (selectedObject >= objects.size())
		return nullptr;

	return &objects[selectedObject];
}

glm::mat4 GetModelMatrix(Object& obj) {

	glm::mat4 m(1);

	m = glm::translate(m, obj.position);

	m = glm::rotate(m, glm::radians(obj.rotation.x), { 1,0,0 });
	m = glm::rotate(m, glm::radians(obj.rotation.y), { 0,1,0 });
	m = glm::rotate(m, glm::radians(obj.rotation.z), { 0,0,1 });

	m = glm::scale(m, obj.scale);

	return m;
}

void gen_cubemap() {

	glGenTextures(1, &cubeFBO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeFBO);

	for (int i = 0; i < 6; i++) {

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

	glRenderbufferStorage(
		GL_RENDERBUFFER,
		GL_DEPTH_COMPONENT24,
		1024,
		1024
	);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		captureRBO
	);
}

void ConvertHDRToCubemap() {

	glViewport(0, 0, 1024, 1024);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glUseProgram(program_hdrToCubemap);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skybox_texture);

	glUniform1i(
		glGetUniformLocation(program_hdrToCubemap,
			"equirectangularMap"),
		0);

	glUniformMatrix4fv(
		glGetUniformLocation(program_hdrToCubemap, "projection"),
		1,
		GL_FALSE,
		glm::value_ptr(captureProjection));

	glBindVertexArray(skyboxVAO);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		captureRBO);

	for (int i = 0; i < 6; i++) {

		glUniformMatrix4fv(
			glGetUniformLocation(program_hdrToCubemap, "view"),
			1,
			GL_FALSE,
			glm::value_ptr(captureViews[i]));

		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			cubeFBO,
			0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, par.w, par.h);
}

void GenSkybox() {

	// my friend LearnOpenGL carries
	float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);

	glBindVertexArray(skyboxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

static glm::vec3 get_cam_pos() {

	glm::vec3 pos;

	pos.x = radius * sin(beta) * cos(alpha);
	pos.y = radius * cos(beta);
	pos.z = radius * sin(beta) * sin(alpha);

	return cam.cam_target + pos;
}

bool RayVsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t) {

	float tmin = 0.0f;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; i++) {

		if (fabs(rayDir[i]) < 0.000001f) {

			if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i])
				return false;
		}
		else {

			float inv = 1.0f / rayDir[i];

			float t1 = (boxMin[i] - rayOrigin[i]) * inv;
			float t2 = (boxMax[i] - rayOrigin[i]) * inv;

			if (t1 > t2)
				std::swap(t1, t2);

			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);

			if (tmin > tmax)
				return false;
		}
	}

	t = tmin;

	return true;
}

void BuildRay(float mouseX, float mouseY, glm::vec3& rayOrigin, glm::vec3& rayDir) {

	float x = (2.0f * mouseX) / par.w - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / par.h;

	glm::vec4 rayClip(x, y, -1.0f, 1.0f);

	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::vec4 rayWorld = glm::inverse(view) * rayEye;

	rayDir = glm::normalize(glm::vec3(rayWorld));

	rayOrigin = get_cam_pos();
}

void select_object(glm::vec3& rayOrigin, glm::vec3& rayDir) {

	float nearest = FLT_MAX;

	selectedObject = -1;

	for (size_t i = 0; i < objects.size(); i++) {

		Object& obj = objects[i];

		glm::vec3 worldMin = obj.min * obj.scale + obj.position;
		glm::vec3 worldMax = obj.max * obj.scale + obj.position;

		float t;

		if (RayVsAABB(rayOrigin, rayDir, worldMin, worldMax, t)) {

			if (t < nearest) {

				nearest = t;
				selectedObject = static_cast<int>(i);
			}
		}
	}
}

static void controls(const bool* keyboard, float angle_step, float radius_step, float dt) {

	glm::vec3 cameraPos = get_cam_pos();

	glm::vec3 forward = glm::normalize(cam.cam_target - cameraPos);
	forward.y = 0.0f;
	forward = glm::normalize(forward);

	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

	float speed = moveSpeed * (radius * 0.09f);

	if (keyboard[SDL_SCANCODE_W])
		cam.cam_target += forward * speed * dt;

	if (keyboard[SDL_SCANCODE_S])
		cam.cam_target -= forward * speed * dt;

	if (keyboard[SDL_SCANCODE_A])
		cam.cam_target -= right * speed * dt;

	if (keyboard[SDL_SCANCODE_D])
		cam.cam_target += right * speed * dt;

	beta = glm::clamp(beta, 0.01f, glm::pi<float>() - 0.01f);

	radius = std::max(radius, 0.5f);

}

static void Create_Grid() {

	std::vector<glm::vec3> vertices;

	const int SIZE = 1000;

	for (int i = -SIZE; i <= SIZE; i++) {

		vertices.push_back(glm::vec3(i * STEP, 0.0f, -SIZE * STEP));
		vertices.push_back(glm::vec3(i * STEP, 0.0f, SIZE * STEP));

		vertices.push_back(glm::vec3(-SIZE * STEP, 0.0f, i * STEP));
		vertices.push_back(glm::vec3(SIZE * STEP, 0.0f, i * STEP));
	}

	gridVertexCount = vertices.size();

	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &gridVBO);

	glBindVertexArray(gridVAO);

	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof(glm::vec3),
		vertices.data(),
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(glm::vec3),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

static void Add_Vertex(Object& obj, const Vertex& v) {

	obj.vertices.push_back(v);
	obj.indices.push_back(obj.indices.size());
}

static void Add_Tri(Object& obj, const Vertex& v0, const Vertex& v1, const Vertex& v2) {

	Add_Vertex(obj, v0);
	Add_Vertex(obj, v1);
	Add_Vertex(obj, v2);
}