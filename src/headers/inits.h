#pragma once

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "helpers.h"

camera cam;
Parser par;

float alpha, beta, radius;


void init_camera() {

	cam.fov = par.fov;

	glm::vec3 offset = cam.cam_position - cam.cam_target;

	radius = glm::length(offset);

	alpha = acos(offset.y / radius);
	beta = atan2(offset.z, offset.x);
}

void init_obj(const char* objFile, Object& obj) {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objFile, "../data/objects/");

	if (!err.empty())
		std::cout << err << std::endl;

	if (!success)
		return;

	obj.vertices.clear();
	obj.indices.clear();

	for (auto& shape : shapes) {

		auto& idx = shape.mesh.indices;

		for (size_t i = 0; i + 2 < idx.size(); i += 3) {

			auto i0 = idx[i + 0];
			auto i1 = idx[i + 1];
			auto i2 = idx[i + 2];

			glm::vec3 p0(attrib.vertices[3 * i0.vertex_index + 0], attrib.vertices[3 * i0.vertex_index + 1], attrib.vertices[3 * i0.vertex_index + 2]);

			glm::vec3 p1(attrib.vertices[3 * i1.vertex_index + 0], attrib.vertices[3 * i1.vertex_index + 1], attrib.vertices[3 * i1.vertex_index + 2]);

			glm::vec3 p2(attrib.vertices[3 * i2.vertex_index + 0], attrib.vertices[3 * i2.vertex_index + 1], attrib.vertices[3 * i2.vertex_index + 2]);

			glm::vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);

			// make sure the texcoord index is valid before accessing the array
			if (i0.texcoord_index >= 0)
				uv0 = { attrib.texcoords[2 * i0.texcoord_index + 0], attrib.texcoords[2 * i0.texcoord_index + 1] };
			if (i1.texcoord_index >= 0)
				uv1 = { attrib.texcoords[2 * i1.texcoord_index + 0], attrib.texcoords[2 * i1.texcoord_index + 1] };
			if (i2.texcoord_index >= 0)
				uv2 = { attrib.texcoords[2 * i2.texcoord_index + 0], attrib.texcoords[2 * i2.texcoord_index + 1] };

			Add_Tri(obj, Vertex{ p0, glm::vec3(0), uv0 }, Vertex{ p1, glm::vec3(0), uv1 }, Vertex{ p2, glm::vec3(0), uv2 });

			//printf("%i", uv0.x);
		}
	}
}
