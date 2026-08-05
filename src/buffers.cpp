#include "headers/buffers.h"

void buffer(Object& obj) {

	glGenVertexArrays(1, &obj.vao);
	glGenBuffers(1, &obj.vbo);
	glGenBuffers(1, &obj.ebo);

	glBindVertexArray(obj.vao);

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
		GL_STATIC_DRAW);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)offsetof(Vertex, position)
	);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)offsetof(Vertex, normal)
	);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(
		2,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(void*)offsetof(Vertex, uv)
	);
	glEnableVertexAttribArray(2);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}