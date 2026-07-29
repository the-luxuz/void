#pragma once

#include "local.h"
#include "classes.h"

const float STEP = 1.0f;

GLuint gridVAO;
GLuint gridVBO;

int gridVertexCount;

static void Create_Grid() {

	std::vector<glm::vec3> vertices;

	const int SIZE = 100;

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