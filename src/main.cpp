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

SDL_Event event;

constexpr float angle_speed = glm::radians(90.0f);
float radius_speed = 20.0f;

enum class view_Mode {
	EDITOR,
	PLAY
};

view_Mode currentViewMode = view_Mode::EDITOR;


void update(SDL_Window* window) {

	update_depthmap();

	if (currentViewMode == view_Mode::EDITOR) {
	
		if ((int)viewportSize.x != viewport_width || (int)viewportSize.y != viewport_height)
			ResizeViewport((int)viewportSize.x, (int)viewportSize.y);


		glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

		glViewport(0, 0, viewport_width, viewport_height);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraPos = get_cam_pos();

		view = glm::lookAt(
			cameraPos,
			cam.cam_target,
			cam.cam_up
		);

		projection = glm::perspective(
			glm::radians(st.fov),
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

	glViewport(0, 0, st.w, st.h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static ImGuiIO& io = ImGui::GetIO();
	(void)io;

	if (currentViewMode == view_Mode::PLAY) {

		io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

		cameraPos = get_cam_pos();

		view = glm::lookAt(
			cameraPos,
			cam.cam_target,
			cam.cam_up
		);

		projection = glm::perspective(
			glm::radians(st.fov),
			(float)st.w / st.h,
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

	if (currentViewMode == view_Mode::EDITOR) {

		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		Docking();
	}

	fps = ImGui::GetIO().Framerate;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	ImGui::Begin("HUD", nullptr, flags);

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Frame: %.3f ms", 1000.0f / fps);

	ImGui::End();

	if (currentViewMode == view_Mode::EDITOR)
		update_imgui_guizmo();

	ImGui::Render();

	if (currentViewMode == view_Mode::EDITOR) {

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

			SDL_Window* backupWindow = SDL_GL_GetCurrentWindow();
			SDL_GLContext backupContext = SDL_GL_GetCurrentContext();

			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			SDL_GL_MakeCurrent(backupWindow, backupContext);
		}
	}

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	//----------------------------------------------------
	// windows

	if (st.last_win_mode != st.win_mode) {
	
		ApplyWindowMode(window);
		init_viewport();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	SDL_GL_SwapWindow(window);
}

void Rebuild() {

	for (int i = 0; i < objects.size(); i++) {

		Object& obj = objects[i];

		// Build_Normals(obj);
		Build_TBN(obj);

		UpdateMeshBuffers(obj);
		UpdateLineBuffers(obj);
	}
}

int main(int argc, char* args[]) {

	init_("../data/scene.bin");

	while (!quit) {

		while (SDL_PollEvent(&event)) {

			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {

			case SDL_EVENT_QUIT:
				quit = true;
				break;

			case SDL_EVENT_KEY_DOWN:

				if (event.key.scancode == SDL_SCANCODE_M) {

					render_modes++;

					if (render_modes > 3)
						render_modes = 0;

					switch (render_modes) {

						case 0:
							tex_appear = true;
							normal_apper = false;
							tangent_apper = false;
							bitangent_apper = false;
							break;

						case 1:
							normal_apper = true;
							tex_appear = false;
							tangent_apper = false;
							bitangent_apper = false;
							break;

						case 2:
							tangent_apper = true;
							tex_appear = false;
							normal_apper = false;
							bitangent_apper = false;
							break;

						case 3:
							bitangent_apper = true;
							tex_appear = false;
							normal_apper = false;
							tangent_apper = false;
							break;
					}


				}

				if (event.key.scancode == SDL_SCANCODE_Y) {
				
					shadow_map_render = !shadow_map_render;

				}

				if (event.key.scancode == SDL_SCANCODE_N) {

					normals_appear = !normals_appear;
					//Rebuild();
				}

				if (event.key.scancode == SDL_SCANCODE_F) {
					face_normal = !face_normal;
					Rebuild();
				}

				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
					quit = true;

				if (currentViewMode == view_Mode::EDITOR) {
				
					if (event.key.scancode == SDL_SCANCODE_BACKSPACE)
					{
						if (selectedObject != -1)
						{
							if (selectedObject < (int)objects.size())
							{
								objects.erase(objects.begin() + selectedObject);
							}
							else
							{
								int lightIndex = selectedObject - (int)objects.size();

								if (lightIndex >= 0 && lightIndex < (int)lightObjects.size())
								{
									lightObjects.erase(lightObjects.begin() + lightIndex);
									lights.erase(lights.begin() + lightIndex);
								}
							}

							selectedObject = -1;
						}
					}
				}

				if (event.key.scancode == SDL_SCANCODE_F11) {

					switch (st.win_mode) {

						case window_mode::WINDOWED:
							st.win_mode = window_mode::WINDOWED_BORDERLESS;
							break;

						case window_mode::WINDOWED_BORDERLESS:
							st.win_mode = window_mode::FULLSCREEN;
							break;

						case window_mode::FULLSCREEN:
							st.win_mode = window_mode::WINDOWED;
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

					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_S) {

						cam.cam_position = cameraPos;

						if(save_outPath != "")
							par.save(save_outPath.c_str(), st, cam, objects, lightObjects, lights);
						else {
						
							nfdu8char_t* outPath = nullptr;

							nfdfilteritem_t filterItem[1] = {
								{ "Text file", "bin" }
							};

							nfdresult_t result = NFD_SaveDialog(
								&outPath,
								filterItem,
								1,
								nullptr,
								"scene.bin"
							);

							if (result == NFD_OKAY) {

								par.save(outPath, st, cam, objects, lightObjects, lights);
								save_outPath = outPath;
								NFD_FreePath(outPath);
							}
						}
					}

					if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_O) {
					
						nfdu8char_t* outPath = nullptr;

						dont_reload_cam = true;

						nfdfilteritem_t filterItem[1] = {
							{ "Text file", "bin" }
						};

						nfdresult_t result = NFD_OpenDialog(
							&outPath,
							filterItem,
							1,
							nullptr
						);

						if (result == NFD_OKAY) {

							std::string path = outPath;

							save_outPath = path;
							open_init(path.c_str(), st);

							NFD_FreePath(outPath);
						}
					}
				}

				break;

			case SDL_EVENT_MOUSE_MOTION:

				if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL]) {

					SDL_HideCursor();
					SDL_SetWindowRelativeMouseMode(window, true);

					cam.alpha -= -(event.motion.xrel * mouseSensitivity); 
					cam.beta -= event.motion.yrel * mouseSensitivity;

					cam.beta = glm::clamp(cam.beta, 0.05f,
						glm::pi<float>() - 0.05f);
				}
				else {
					SDL_ShowCursor();
					SDL_SetWindowRelativeMouseMode(window, false);
				}

				break;

			case SDL_EVENT_KEY_UP:

				if (event.key.scancode == SDL_SCANCODE_LCTRL)
					SDL_WarpMouseInWindow(window, st.w / 2.0f, st.h / 2.0f);

				break;

			case SDL_EVENT_MOUSE_WHEEL:

				cam.radius -= (event.wheel.y * 10);

				cam.radius = std::max(cam.radius, 1.0f);

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

	clean();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}