#include "headers/buffers.h"
#include "headers/helpers.h"

void buffer(Object& objects) {

	if (objects.vao)
		glDeleteVertexArrays(1, &objects.vao);

	if (objects.vbo)
		glDeleteBuffers(1, &objects.vbo);

	if (objects.ebo)
		glDeleteBuffers(1, &objects.ebo);

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

	CreateNormalBuffers(objects);

	glBindVertexArray(0);
}

void CreateNormalBuffers(Object& obj) {

	if (obj.normalTBVAO) {
	
		glGenVertexArrays(1, &obj.normalTBVAO);
		glGenBuffers(1, &obj.normalTBVBO);
		CreateLineBuffer(obj.normalTBVAO, obj.normalTBVBO, obj.normalLinesTBN);
	}

	if (obj.tangentVAO) {

		glGenVertexArrays(1, &obj.tangentVAO);
		glGenBuffers(1, &obj.tangentVBO);
		CreateLineBuffer(obj.tangentVAO, obj.tangentVBO, obj.tangentLines);
	}

	if (obj.bitangentVAO) {

		glGenVertexArrays(1, &obj.bitangentVAO);
		glGenBuffers(1, &obj.bitangentVBO);
		CreateLineBuffer(obj.bitangentVAO, obj.bitangentVBO, obj.bitangentLines);
	}
}

void UpdateLineBuffers(Object& obj) {

	CreateLineBuffer(obj.normalTBVAO, obj.normalTBVBO, obj.normalLinesTBN);

	CreateLineBuffer(obj.tangentVAO, obj.tangentVBO, obj.tangentLines);

	CreateLineBuffer(obj.bitangentVAO, obj.bitangentVBO, obj.bitangentLines);
}

void UpdateMeshBuffers(Object& obj) {

	if (obj.vbo)
		glDeleteBuffers(1, &obj.vbo);

	if (obj.ebo)
		glDeleteBuffers(1, &obj.ebo);

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

void CreateVideoQuad(VideoQuad& quad) {

	glGenVertexArrays(1, &quad.vao);
	glGenBuffers(1, &quad.vbo);
	glGenBuffers(1, &quad.ebo);

	glBindVertexArray(quad.vao);

	glBindBuffer(GL_ARRAY_BUFFER, quad.vbo);

	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(quad.vertices),
		quad.vertices,
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.ebo);

	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		sizeof(quad.indices),
		quad.indices,
		GL_STATIC_DRAW
	);

	// Position
	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		4 * sizeof(float),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	// UV
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		4 * sizeof(float),
		(void*)(2 * sizeof(float))
	);

	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}