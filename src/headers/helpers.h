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

static glm::vec3 get_cam_pos() {

	glm::vec3 pos;

	pos.x = radius * sin(beta) * cos(alpha);
	pos.y = radius * cos(beta);
	pos.z = radius * sin(beta) * sin(alpha);

	return cam.cam_target + pos;
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