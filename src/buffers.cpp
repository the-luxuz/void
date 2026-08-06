#include "headers/buffers.h"
#include "headers/helpers.h"

void buffer(Object& objects) {

	glGenVertexArrays(1, &objects.vao);
	glGenVertexArrays(1, &objects.vao);
	glGenBuffers(1, &objects.vbo);
	glGenBuffers(1, &objects.ebo);

	glBindVertexArray(objects.vao);

	glBindBuffer(GL_ARRAY_BUFFER, objects.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		objects.vertices.size() * sizeof(Vertex),
		objects.vertices.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects.ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		objects.indices.size() * sizeof(unsigned int),
		objects.indices.data(),
		GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, position)
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, uv)
	);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, normal)
	);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(
		5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, tangent)
	);

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(
		6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, bitangent)
	);

	glBindVertexArray(0);
}

void CreateNormalBuffers(Object& obj) {

	glGenVertexArrays(1, &obj.normalTBVAO);
	glGenVertexArrays(1, &obj.tangentVAO);
	glGenVertexArrays(1, &obj.bitangentVAO);

	glGenBuffers(1, &obj.normalTBVBO);
	glGenBuffers(1, &obj.tangentVBO);
	glGenBuffers(1, &obj.bitangentVBO);

	CreateLineBuffer(obj.normalTBVAO, obj.normalTBVBO, obj.normalLinesTBN);
	CreateLineBuffer(obj.tangentVAO, obj.tangentVBO, obj.tangentLines);
	CreateLineBuffer(obj.bitangentVAO, obj.bitangentVBO, obj.bitangentLines);
}

void UpdateLineBuffers(Object& obj) {

	CreateLineBuffer(obj.normalTBVAO, obj.normalTBVBO, obj.normalLinesTBN);

	CreateLineBuffer(obj.tangentVAO, obj.tangentVBO, obj.tangentLines);

	CreateLineBuffer(obj.bitangentVAO, obj.bitangentVBO, obj.bitangentLines);
}

void UpdateMeshBuffers(Object& obj) {

	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		obj.vertices.size() * sizeof(Vertex),
		obj.vertices.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		obj.indices.size() * sizeof(GLuint),
		obj.indices.data(),
		GL_STATIC_DRAW
	);
}