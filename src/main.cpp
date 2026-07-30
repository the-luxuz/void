#define TINYOBJLOADER_IMPLEMENTATION

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imguizmo.h"

#include "local.h"
#include "classes.h"
#include "Parser.h"
#include "inits.h"
#include "helpers.h"
#include "programs.h"
#include "buffers.h"
#include "textures.h"

float totalTime = 0.0f;

static int currentMesh = 0;

bool useSnap;

float* snap;

glm::mat4 m;

static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

const char* meshes[] = {
	"Plane",
	"Sphere",
	"Cube",
	"Cylinder"
};

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	init_camera();

	init_skybox();

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

	glUseProgram(program_grid);

	glm::mat4 model(1.0f);

	glm::vec3 gridPos(floor(cameraPos.x / STEP) * STEP, 0.0f, floor(cameraPos.z / STEP) * STEP);

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
	// obj

	glUseProgram(program);

	for (Object& obj : objects) {

		glm::mat4 model = GetModelMatrix(obj);

		glm::mat4 mvp = projection * view * model;

		glUniformMatrix4fv(
			glGetUniformLocation(program, "uMVP"),
			1,
			GL_FALSE,
			glm::value_ptr(mvp)
		);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, obj.obj_texture);

		glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

		glBindVertexArray(obj.vao);

		glDrawElements(
			GL_TRIANGLES,
			obj.indices.size(),
			GL_UNSIGNED_INT,
			nullptr
		);
	}

	//----------------------------------------------------
	// skybox

	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);

	glUseProgram(program_skybox);

	glm::mat4 skyView = glm::mat4(glm::mat3(view));

	glUniformMatrix4fv(glGetUniformLocation(program_skybox, "view"), 1, GL_FALSE, glm::value_ptr(skyView));

	glUniformMatrix4fv(glGetUniformLocation(program_skybox, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeFBO);

	glUniform1i(glGetUniformLocation(program_skybox, "skybox"), 0);

	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	//----------------------------------------------------
	// imgui / imguizmo

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	Object* obj = GetSelectedObject();

	if (obj) {

		m = GetModelMatrix(*obj);

		ImGuizmo::BeginFrame();

		//ImGuizmo::SetDrawlist();

		ImGuizmo::SetOrthographic(false);

		ImGuizmo::SetRect(0, 0, (float)par.w, (float)par.h );

		float moveSnap[3] = { 12.5, 12.5, 12.5 };
		float scaleSnap[3] = { 12.5f, 12.5f, 12.5f };
		float rotateSnap[3] = { 11.25, 11.25, 11.25 };

		snap = nullptr;

		useSnap = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT];

		if (useSnap) {

			switch (currentOperation) {

			case ImGuizmo::TRANSLATE:
				snap = moveSnap;
				break;

			case ImGuizmo::ROTATE:
				snap = rotateSnap;
				break;

			case ImGuizmo::SCALE:
				snap = scaleSnap;
				break;
			}
		}

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentOperation, ImGuizmo::LOCAL, glm::value_ptr(m), nullptr, snap);

		if (ImGuizmo::IsUsing()) {

			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

			if (useSnap && currentOperation == ImGuizmo::ROTATE) {

				auto Snap = [](float value, float step) {
						return roundf(value / step) * step;
				};

				rotation.x = Snap(rotation.x, rotateSnap[0]);
				rotation.y = Snap(rotation.y, rotateSnap[1]);
				rotation.z = Snap(rotation.z, rotateSnap[2]);
			}

			obj->position = translation;
			obj->rotation = rotation;
			obj->scale = scale;
		}

	}

	ImGui::Begin("Create Object");

	ImGui::Combo("Mesh", &currentMesh, meshes, IM_ARRAYSIZE(meshes));

	if (ImGui::Button("Create")) {

		Object obj;

		switch (currentMesh) {

		case 0:
			init_obj("../data/objects/plane.obj", obj);
			obj.name = "Plane";
			break;

		case 1:
			init_obj("../data/objects/sphere.obj", obj);
			obj.name = "Sphere";
			break;

		case 2:
			init_obj("../data/objects/cube.obj", obj);
			obj.name = "Cube";
			break;

		case 3:
			init_obj("../data/objects/cylinder.obj", obj);
			obj.name = "Cylinder";
			break;
		}

		buffer(obj);

		obj.position = glm::vec3(0.0f);
		obj.rotation = glm::vec3(0.0f);
		obj.scale = glm::vec3(50.0f);

		objects.push_back(obj);
		selectedObject = (int)(objects.size() - 1);
	}

	ImGui::End();

	//----------------------------------------------------
	// tools

	ImGui::Begin("Tools");

	if (ImGui::RadioButton("Move", currentOperation == ImGuizmo::TRANSLATE))
		currentOperation = ImGuizmo::TRANSLATE;

	ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE))
		currentOperation = ImGuizmo::ROTATE;

	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE))
		currentOperation = ImGuizmo::SCALE;

	ImGui::End();

	//----------------------------------------------------
	// hierarchy

	ImGui::SetNextWindowPos(ImVec2(par.w - 300, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, par.h), ImGuiCond_Always);

	ImGuiWindowFlags h_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("Hierarchy", nullptr, h_flags);

	for (size_t i = 0; i < objects.size(); i++) {

		ImGui::PushID((int)i);

		bool selected = (selectedObject == (int)i);

		if (ImGui::Selectable(objects[i].name.c_str(), selected))
			selectedObject = i;

		ImGui::PopID();
	}

	ImGui::End();

	static float fps = 0.0f;

	fps = ImGui::GetIO().Framerate;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.35f);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("HUD", nullptr, flags);

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Frame: %.3f ms", 1000.0f / fps);

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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