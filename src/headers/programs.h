#pragma once

#include "local.h"

GLuint program;
GLuint program_grid;

GLuint create_shaders(GLuint pro, const char* vertexShaderPath, const char* fragmentShaderPath) {
	
	//------------------------------------------------------------
	// vertex shader

	std::ifstream vsFile(vertexShaderPath);

	std::stringstream vsStream;
	vsStream << vsFile.rdbuf();

	std::string vsSource = vsStream.str();

	const char* vsCode = vsSource.c_str();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertexShader, 1, &vsCode, nullptr);
	glCompileShader(vertexShader);

	//------------------------------------------------------------
	// fragment shader

	std::ifstream fsFile(fragmentShaderPath);

	std::stringstream fsStream;
	fsStream << fsFile.rdbuf();

	std::string fsSource = fsStream.str();

	const char* fsCode = fsSource.c_str();

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragmentShader, 1, &fsCode, nullptr);
	glCompileShader(fragmentShader);

	pro = glCreateProgram();

	glAttachShader(pro, vertexShader);
	glAttachShader(pro, fragmentShader);

	glLinkProgram(pro);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return pro;

}

void create_programs() {

	program = create_shaders(program, "../data/shaders/vertex.vert", "../data/shaders/fragment.frag");

	program_grid = create_shaders(program_grid, "../data/shaders/vertex_grid.vert", "../data/shaders/fragment_grid.frag");
}