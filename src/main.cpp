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

float totalTime = 0.0f;

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

	//----------------------------------------------------
	// grid

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
	// obj

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

	//----------------------------------------------------
	// imgui / imguizmo

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Hey");

	ImGui::Text("ImGui works!");

	ImGui::End();

	glm::mat4 m;

	if (selectedObject != nullptr) {

		m = glm::mat4(1.0f);

		m = glm::translate(m, selectedObject->position);

		m = glm::rotate(m, glm::radians(selectedObject->rotation.x), glm::vec3(1, 0, 0));

		m = glm::rotate(m, glm::radians(selectedObject->rotation.y), glm::vec3(0, 1, 0));

		m = glm::rotate(m, glm::radians(selectedObject->rotation.z), glm::vec3(0, 0, 1));

		m = glm::scale(m, selectedObject->scale);

		ImGuizmo::BeginFrame();

		//ImGuizmo::SetDrawlist();

		ImGuizmo::SetOrthographic(false);

		ImGuizmo::SetRect(0, 0, (float)par.w, (float)par.h );

		ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(m));

		if (ImGuizmo::IsUsing()) {

			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

			selectedObject->position = translation;
			selectedObject->rotation = rotation;
			selectedObject->scale = scale;
		}

	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(window);
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