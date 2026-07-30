#define TINYOBJLOADER_IMPLEMENTATION

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "inits.h"
#include "helpers.h"
#include "programs.h"
#include "buffers.h"
#include "textures.h"
#include "updates.h"

float totalTime = 0.0f;

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	init_camera();

	init_grid();

	init_skybox(par.skybox.c_str());

	read_textures_path();

	for (int i = 0; i < par.objects.size(); i++) {
		
		Object obj;

		std::string fullPath = "../" + par.objects[i].mesh;
		init_obj(fullPath.c_str(), obj);

		buffer(obj);

		obj.mesh = par.objects[i].mesh;

		obj.obj_texture = LoadObjTexture(par.objects[i].tex);

		obj.name = par.objects[i].obj_name;

		obj.position = par.objects[i].position;
		obj.scale = par.objects[i].scale;
		obj.rotation = par.objects[i].rotation;

		objects.push_back(obj);
	}
}


void update(SDL_Window* window) {

	glViewport(0, 0, par.w, par.h);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::vec3 cameraPos = get_cam_pos();

	view = glm::lookAt(
		cameraPos,
		cam.cam_target,
		cam.cam_up
	);

	projection = glm::perspective(
		glm::radians(par.fov),
		(float)par.w / par.h,
		cam.nearPlane,
		cam.farPlane
	);

	if (par.last_win_mode != par.win_mode) {

		switch (par.win_mode) {

		case Parser::window_mode::WINDOWED:
			SDL_SetWindowFullscreen(window, false);
			SDL_RestoreWindow(window);
			SDL_SetWindowBordered(window, true);
			break;

		case Parser::window_mode::WINDOWED_BORDERLESS:
			SDL_SetWindowFullscreen(window, false);
			SDL_SetWindowBordered(window, false);
			SDL_MaximizeWindow(window);
			break;

		case Parser::window_mode::FULLSCREEN:
			SDL_SetWindowFullscreen(window, true);
			break;
		}

		int w, h;
		SDL_GetWindowSizeInPixels(window, &w, &h);

		par.w = w;
		par.h = h;

		par.last_win_mode = par.win_mode;
	}

	//----------------------------------------------------
	// grid

	update_grid(cameraPos);

	//----------------------------------------------------
	// obj

	update_objects();

	//----------------------------------------------------
	// skybox

	update_skybox();

	//----------------------------------------------------
	// imgui / imguizmo

	update_imgui_guizmo();

	//----------------------------------------------------

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	SDL_GL_SwapWindow(window);
}

int main(int argc, char* args[]) {

	par.load("../data/settings.txt");

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window;

	if (par.win_mode == Parser::window_mode::FULLSCREEN)
		window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_FULLSCREEN);
	else if(par.win_mode == Parser::window_mode::WINDOWED_BORDERLESS)
		window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_BORDERLESS);
	else
		window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_OPENGL);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {

		std::cout << glewGetErrorString(err) << '\n';
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init("#version 330");

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

			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {

			case SDL_EVENT_QUIT:
				quit = true;
				break;

			case SDL_EVENT_KEY_DOWN:

				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
					quit = true;

				if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {

					if (selectedObject != -1) {

						for (size_t i = 0; i < objects.size(); i++) {

							if (selectedObject == (int)i) {

								objects.erase(objects.begin() + i);
								selectedObject = -1;
								break;
							}
						}
					}
				}

				if(event.key.scancode == SDL_SCANCODE_F11) {

					if(par.win_mode == Parser::window_mode::FULLSCREEN)
						par.win_mode = Parser::window_mode::WINDOWED;
					else if (par.win_mode == Parser::window_mode::WINDOWED)
						par.win_mode = Parser::window_mode::WINDOWED_BORDERLESS;
					else
						par.win_mode = Parser::window_mode::FULLSCREEN;
				}

				if (event.key.scancode == SDL_SCANCODE_F5) {
					read_textures_path();
				}

				break;

			case SDL_EVENT_MOUSE_MOTION:

				if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL]) {

					SDL_HideCursor();
					SDL_SetWindowRelativeMouseMode(window, true);

					alpha -= -(event.motion.xrel * mouseSensitivity); 
					beta -= event.motion.yrel * mouseSensitivity;

					beta = glm::clamp(beta, 0.05f,
						glm::pi<float>() - 0.05f);
				}
				else {
					SDL_ShowCursor();
					SDL_SetWindowRelativeMouseMode(window, false);
				}

				break;

			case SDL_EVENT_KEY_UP:

				if (event.key.scancode == SDL_SCANCODE_LCTRL)
					SDL_WarpMouseInWindow(window, par.w / 2.0f, par.h / 2.0f);

				break;

			case SDL_EVENT_MOUSE_WHEEL:

				radius -= (event.wheel.y * 10);

				radius = std::max(radius, 1.0f);

				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:

				if (event.button.button == SDL_BUTTON_LEFT) {

					if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
						break;

					if (ImGui::GetIO().WantCaptureMouse)
						break;

					BuildRay(event.button.x, event.button.y, rayOrigin, rayDir);

					select_object(rayOrigin, rayDir);

					/*if (selectedObject != nullptr) {
						std::cout << "selected" << std::endl;
					}*/
				}

				if (event.button.button == SDL_BUTTON_RIGHT) {
					selectedObject = -1;
				}

				break;
			}

		}

		const bool* keyboard = SDL_GetKeyboardState(nullptr);

		controls(keyboard, angle_speed, radius_speed, dt);

		totalTime += dt;

		update(window);
	}

	glDeleteProgram(program);
	glDeleteProgram(program_grid);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}