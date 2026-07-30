#pragma once

#include "local.h"
#include "classes.h"

camera cam;
Parser par;

float alpha, beta, radius;

const float STEP = 25.0f;

GLuint gridVAO;
GLuint gridVBO;

int gridVertexCount;

float moveSpeed = 10.0f;
float mouseSensitivity = 0.005f;

Object* selectedObject = nullptr;

enum class view_Mode {
	EDITOR,
	PLAY
};

view_Mode currentViewMode = view_Mode::EDITOR;

glm::mat4 view;
glm::mat4 projection;

glm::vec3 rayOrigin;
glm::vec3 rayDir;

std::vector<Object> objects;

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

	Object* selected = nullptr;
	float nearest = FLT_MAX;

	for (Object& obj : objects) {

		glm::vec3 worldMin = obj.min * obj.scale + obj.position;
		glm::vec3 worldMax = obj.max * obj.scale + obj.position;

		float t;

		if (RayVsAABB(rayOrigin, rayDir, worldMin, worldMax, t)) {

			if (t < nearest) {

				nearest = t;
				selected = &obj;

				selectedObject = selected;
			}
		}
		else {
			selectedObject = nullptr;
			selected = nullptr;
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