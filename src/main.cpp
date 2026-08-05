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

static float fps = 0.0f;

Object objCopy;
bool hasCopy = false;

SDL_DisplayID display;
SDL_Rect rect;

enum class view_Mode {
	EDITOR,
	PLAY
};

view_Mode currentViewMode = view_Mode::EDITOR;

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	init_camera();

	init_grid();

	init_skybox(par.skybox.c_str());

	init_viewport();

	read_textures_path();

	ReloadFolder();

	for (int i = 0; i < par.objects.size(); i++) {
		
		Object obj;

		std::string fullPath = "../" + par.objects[i].mesh;
		init_obj(fullPath.c_str(), obj);

		buffer(obj);

		obj.original_vertices = obj.vertices;
		obj.original_indices = obj.indices;

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

	if (currentViewMode == view_Mode::EDITOR) {
	
	
		if ((int)viewportSize.x != viewport_width || (int)viewportSize.y != viewport_height)
			ResizeViewport((int)viewportSize.x, (int)viewportSize.y);

		glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

		glViewport(0, 0, viewport_width, viewport_height);

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
			(float)viewport_width / viewport_height,
			cam.nearPlane,
			cam.farPlane
		);

		if (currentViewMode == view_Mode::EDITOR) {
		
			//----------------------------------------------------
			// grid

			update_grid(cameraPos);

		}

		//----------------------------------------------------
		// obj

		update_objects();

		//----------------------------------------------------
		// skybox

		update_skybox();

		//----------------------------------------------------

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	//----------------------------------------------------

	glViewport(0, 0, par.w, par.h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (currentViewMode == view_Mode::PLAY) {

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

		//----------------------------------------------------
		// obj

		update_objects();

		//----------------------------------------------------
		// skybox

		update_skybox();

		//----------------------------------------------------

	}

	//----------------------------------------------------
	// imgui / imguizmo

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	fps = ImGui::GetIO().Framerate;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.35f);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("HUD", nullptr, flags);

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Frame: %.3f ms", 1000.0f / fps);

	ImGui::End();

	if (currentViewMode == view_Mode::EDITOR)
		update_imgui_guizmo();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	//----------------------------------------------------
	// windows

	if (par.last_win_mode != par.win_mode)
		ApplyWindowMode(window);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	SDL_GL_SwapWindow(window);
}

int main(int argc, char* args[]) {

	par.load("../data/settings.txt");

	windowedSize = { par.w, par.h };

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_OPENGL);

	ApplyWindowMode(window);

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!par.VSync)
		SDL_GL_SetSwapInterval(0);
	else
		SDL_GL_SetSwapInterval(1);
	
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

	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init("#version 330");

	create_programs();

	init();

	bool quit = false;

	SDL_Event event;

	constexpr float angle_speed = glm::radians(90.0f);
	float radius_speed = 20.0f;

	while (!quit) {

		while (SDL_PollEvent(&event)) {

			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {

			case SDL_EVENT_QUIT:
				quit = true;
				break;

			case SDL_EVENT_KEY_DOWN:

				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
					quit = true;

				if (currentViewMode == view_Mode::EDITOR) {
				
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
				}

				if (event.key.scancode == SDL_SCANCODE_F11) {

					switch (par.win_mode) {

						case Parser::window_mode::WINDOWED:
							par.win_mode = Parser::window_mode::WINDOWED_BORDERLESS;
							break;

						case Parser::window_mode::WINDOWED_BORDERLESS:
							par.win_mode = Parser::window_mode::FULLSCREEN;
							break;

						case Parser::window_mode::FULLSCREEN:
							par.win_mode = Parser::window_mode::WINDOWED;
							break;
					}
				}

				if (event.key.scancode == SDL_SCANCODE_F5) {
					read_textures_path();
				}

				if (event.key.scancode == SDL_SCANCODE_T) {

					if(currentViewMode == view_Mode::EDITOR) {
						currentViewMode = view_Mode::PLAY;
					}
					else {
						currentViewMode = view_Mode::EDITOR;
					}
				}

				if (currentViewMode == view_Mode::EDITOR) {
					
					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_Z) {

						if (selectedObject != -1 && !objects[selectedObject].undoStack.empty()) {

							objects[selectedObject].redoStack.push_back(CaptureState(objects[selectedObject]));
							ApplyState(objects[selectedObject], objects[selectedObject].undoStack.back());
							objects[selectedObject].undoStack.pop_back();
						}
					}

					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_Y) {

						if (selectedObject != -1 && !objects[selectedObject].redoStack.empty()) {

							objects[selectedObject].undoStack.push_back(CaptureState(objects[selectedObject]));
							ApplyState(objects[selectedObject], objects[selectedObject].redoStack.back());
							objects[selectedObject].redoStack.pop_back();
						}
					}

					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_C) {

						if (selectedObject != -1) {

							objCopy = objects[selectedObject];
							hasCopy = true;
						}
					}

					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_V) {

						if (hasCopy) {

							objects.push_back(objCopy);
							selectedObject = objects.size() - 1;
						}
					}
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

					//if (ImGui::GetIO().WantCaptureMouse)
					//	break;

					float mx = event.button.x - viewportPos.x;
					float my = event.button.y - viewportPos.y;

					if (mx < 0 || my < 0 || mx > viewport_width || my > viewport_height)
						break;

					BuildRay(mx, my, rayOrigin, rayDir);

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

		controls(keyboard, angle_speed, radius_speed, 1 / fps);

		totalTime += 1.0f / fps;

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