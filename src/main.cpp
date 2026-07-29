#define TINYOBJLOADER_IMPLEMENTATION

#include "headers/local.h"
#include "headers/classes.h"
#include "headers/Parser.h"
#include "headers/inits.h"
#include "headers/helpers.h"
#include "headers/programs.h"

std::vector<Object> objects;

float totalTime = 0.0f;

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
		(void*)offsetof(Vertex, position));

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	init_camera();

	Create_Grid();

	for (int i = 0; i < par.objects.size(); i++) {
		
		Object obj;

		std::string fullPath = "../" + par.objects[i].mesh;
		init_obj(fullPath.c_str(), obj);

		buffer(obj);

		obj.mesh = par.objects[i].mesh;

		obj.position = par.objects[i].position;
		obj.scale = par.objects[i].scale;
		obj.rotation = par.objects[i].rotation;

		std::cout << obj.mesh << std::endl;

		std::cout << obj.position.x << "  " << obj.position.y << "  " << obj.position.z << "  " << std::endl;
		std::cout << obj.scale.x << "  " << obj.scale.y << "  " << obj.scale.z << "  " << std::endl;
		std::cout << obj.rotation.x << "  " << obj.rotation.y << "  " << obj.rotation.z << "  " << std::endl;

		objects.push_back(obj);
	}

}


static glm::vec3 get_cam_pos() {

	glm::vec3 pos;

	pos.x = radius * sin(beta) * cos(alpha);
	pos.y = radius * cos(beta);
	pos.z = radius * sin(beta) * sin(alpha);

	return cam.cam_target + pos;
}


void update(SDL_Window* window) {

	glViewport(0, 0, par.w, par.h);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec3 cameraPos = get_cam_pos();

	glm::mat4 view = glm::lookAt(
		cameraPos,
		cam.cam_target,
		cam.cam_up
	);

	glm::mat4 projection = glm::perspective(
		glm::radians(par.fov),
		(float)par.w / par.h,
		cam.nearPlane,
		cam.farPlane
	);

	//----------------------------------------------------
	// GRID DEL MUNDO
	//----------------------------------------------------

	glUseProgram(program_grid);

	glm::mat4 model(1.0f);

	glm::vec3 gridPos(
		floor(cameraPos.x / STEP) * STEP,
		0.0f,
		floor(cameraPos.z / STEP) * STEP
	);

	model = glm::translate(glm::mat4(1.0f), gridPos);

	glm::mat4 mvp = projection * view * model;

	glUniformMatrix4fv(
		glGetUniformLocation(program_grid, "uMVP"),
		1,
		GL_FALSE,
		glm::value_ptr(mvp)
	);

	glBindVertexArray(gridVAO);

	glDrawArrays(
		GL_LINES,
		0,
		gridVertexCount
	);

	glBindVertexArray(0);

	//----------------------------------------------------
	// OBJETOS
	//----------------------------------------------------

	glUseProgram(program);

	for (Object& obj : objects) {

		glm::mat4 model(1.0f);

		model = glm::translate(model, obj.position);

		model = glm::rotate(model,
			glm::radians(obj.rotation.x),
			glm::vec3(1, 0, 0));

		model = glm::rotate(model,
			glm::radians(obj.rotation.y),
			glm::vec3(0, 1, 0));

		model = glm::rotate(model,
			glm::radians(obj.rotation.z),
			glm::vec3(0, 0, 1));

		model = glm::scale(model, obj.scale);

		glm::mat4 mvp = projection * view * model;

		glUniformMatrix4fv(
			glGetUniformLocation(program, "uMVP"),
			1,
			GL_FALSE,
			glm::value_ptr(mvp)
		);

		glBindVertexArray(obj.vao);

		glDrawElements(
			GL_TRIANGLES,
			obj.indices.size(),
			GL_UNSIGNED_INT,
			nullptr
		);

		glBindVertexArray(0);
	}

	SDL_GL_SwapWindow(window);
}

float moveSpeed = 10.0f;
float mouseSensitivity = 0.005f;

static void controls(const bool* keyboard, float angle_step, float radius_step, float dt) {

	glm::vec3 cameraPos = get_cam_pos();

	glm::vec3 forward = glm::normalize(cam.cam_target - cameraPos);
	forward.y = 0.0f;
	forward = glm::normalize(forward);

	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

	if (keyboard[SDL_SCANCODE_W])
		cam.cam_target += forward * moveSpeed * dt;

	if (keyboard[SDL_SCANCODE_S])
		cam.cam_target -= forward * moveSpeed * dt;

	if (keyboard[SDL_SCANCODE_A])
		cam.cam_target -= right * moveSpeed * dt;

	if (keyboard[SDL_SCANCODE_D])
		cam.cam_target += right * moveSpeed * dt;

	beta = glm::clamp(beta, 0.01f, glm::pi<float>() - 0.01f);

	radius = std::max(radius, 0.5f);

}

int main(int argc, char* args[]) {

	par.load("../data/settings.txt");

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_OPENGL);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {

		std::cout << glewGetErrorString(err) << '\n';
		return -1;
	}

	create_programs();

	init();

	bool quit = false;

	SDL_Event event;

	constexpr float angle_speed = glm::radians(90.0f);
	float radius_speed = 20.0f;

	Uint64 previousCounter = SDL_GetPerformanceCounter();
	Uint64 frequency = SDL_GetPerformanceFrequency();

	while (!quit) {

		Uint64 currentCounter = SDL_GetPerformanceCounter();

		float dt = static_cast<float>(currentCounter - previousCounter) /
			static_cast<float>(frequency);

		previousCounter = currentCounter;

		while (SDL_PollEvent(&event)) {

			switch (event.type) {

			case SDL_EVENT_QUIT:
				quit = true;
				break;

			case SDL_EVENT_KEY_DOWN:

				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
					quit = true;

				break;

			case SDL_EVENT_MOUSE_MOTION:

				if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL]) {

					alpha -= -(event.motion.xrel * mouseSensitivity); 
					beta -= event.motion.yrel * mouseSensitivity;

					beta = glm::clamp(beta, 0.05f,
						glm::pi<float>() - 0.05f);
				}

				break;

			case SDL_EVENT_MOUSE_WHEEL:

				radius -= event.wheel.y;

				radius = std::max(radius, 1.0f);

				break;
			}

		}

		const bool* keyboard = SDL_GetKeyboardState(nullptr);

		controls(keyboard, angle_speed, radius_speed, dt);

		totalTime += dt;

		update(window);
	}

	glDeleteProgram(program);

	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}