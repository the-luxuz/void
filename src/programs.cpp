#include "headers/programs.h"
#include "headers/textures.h"

//--------------------------------------------------------------------------
// global variables

// programs
GLuint program;
GLuint program_line;
GLuint program_grid;
GLuint program_depth;
GLuint program_skybox;
GLuint program_hdrToCubemap;

//depth framebuffer
GLuint depthFBO[8];

//--------------------------------------------------------------------------
// Helper create shader

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

//--------------------------------------------------------------------------
// init programs

void create_programs() {

	program = create_shaders(program, "../data/shaders/vertex.vert", "../data/shaders/fragment.frag");

	program_line = create_shaders(program_line, "../data/shaders/vertex_line.vert", "../data/shaders/fragment_line.frag");

	program_depth = create_shaders(program_depth, "../data/shaders/vertex_depth.vert", "../data/shaders/fragment_depth.frag");

	program_grid = create_shaders(program_grid, "../data/shaders/vertex_grid.vert", "../data/shaders/fragment_grid.frag");

	program_skybox = create_shaders(program_skybox, "../data/shaders/vertex_skybox.vert", "../data/shaders/fragment_skybox.frag");

	program_hdrToCubemap = create_shaders(program_hdrToCubemap, "../data/shaders/vertex_hdrToCubemap.vert", "../data/shaders/fragment_hdrToCubemap.frag");
}