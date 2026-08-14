#include "headers/helpers.h"

//--------------------------------------------------------------------------
// global variables

// structures
camera cam;
Parser par;
Settings st;

// buffers
GLuint viewportFBO;
GLuint dynamic_cubemap;

GLuint envCubemap;

GLuint captureFBO;
GLuint captureRBO;

GLuint gridVAO;
GLuint gridVBO;

GLuint skyVAO;
GLuint skyVBO;

GLuint skyboxVAO;
GLuint skyboxVBO;

// viewport
ImVec2 viewportPos;
ImVec2 viewportSize;

// vectors
std::vector<Object> objects;
std::vector<Light> lights;
std::vector<Object> lightObjects;

std::vector<Object> saved_objects;
std::vector<Object> saved_light_objects;
std::vector<Light> saved_lights;

// light view-proj matrices
glm::mat4 lightVP[MAX_LIGHTS];

glm::mat4 view;
glm::mat4 projection;

// selected object and ray casting
int selectedObject = -1;

glm::vec3 rayOrigin;
glm::vec3 rayDir;

// movement
float moveSpeed = 10.0f;
float mouseSensitivity = 0.005f;

// skybox
glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] = {

    glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1,0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1,0))
};

// grid
const float STEP = 25.0f;
const float cellSize = 90.0f;
int gridVertexCount;

// folder and textures
std::string dirPath = "data/textures";

std::filesystem::path rootFolder = "../data";
std::filesystem::path currentFolder = rootFolder;

std::vector<std::filesystem::path> pathTextures;

bool reloadFolder = false;
std::filesystem::path newFolder;
std::vector<std::filesystem::directory_entry> folderEntries;

std::string save_outPath = "";

// unordered mapo names
std::unordered_map<std::string, int> objectNameCounters;

// undo/redo
Object objCopy;
Light lightCopy;

bool hasObjectCopy;
bool hasLightCopy;

ObjectState prevState;
LightState prevLightState;

// camera pos
glm::vec3 cameraPos;

// windowed size
glm::ivec2 windowedSize;

// flags
bool dont_reload_cam = false;

bool face_normal = false;

bool normals_appear;

bool shadow_map_render = true;

bool quit = false;

// render modes
int render_modes = 0;

view_Mode currentViewMode = view_Mode::EDITOR;

view_Mode prevViewMode = currentViewMode;

// tools
Tools currentTool = Tools::TRANSLATE;

//docking
static bool showGraphicsSettings = false;

bool camera_update = true;

//--------------------------------------------------------------------------
// Load scene in the good way

bool load_scene_ok(const char* scene) {

	selectedObject = -1;

	for (auto& o : objects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DestroyBuffers(o);

		o.vao = 0;
		o.vbo = 0;
		o.ebo = 0;

		o.normalVAO = 0;
		o.normalVBO = 0;

		o.normalTBVAO = 0;
		o.normalTBVBO = 0;

		o.tangentVAO = 0;
		o.tangentVBO = 0;

		o.bitangentVAO = 0;
		o.bitangentVBO = 0;

	}

	for (auto& o : lightObjects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DestroyBuffers(o);

		o.vao = 0;
		o.vbo = 0;
		o.ebo = 0;

		o.normalVAO = 0;
		o.normalVBO = 0;

		o.normalTBVAO = 0;
		o.normalTBVBO = 0;

		o.tangentVAO = 0;
		o.tangentVBO = 0;

		o.bitangentVAO = 0;
		o.bitangentVBO = 0;
	}

	for (auto& o : lights) {

		o.undoStack.clear();
		o.redoStack.clear();
	}

	objects.clear();
	lightObjects.clear();
	lights.clear();

	objectNameCounters.clear();

	std::ifstream in(scene, std::ios::binary);

	if (par.load_scene(in, lightObjects))
		init();

	return true;
}

//--------------------------------------------------------------------------
// ImGui docking

void Docking() {
	
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("DockSpace", nullptr, flags);

	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id);

	static bool first_time = true;

	if (first_time) {

		first_time = false;

		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

		static ImGuiID dock_main = dockspace_id;

		static ImGuiID dock_left;
		static ImGuiID dock_right;
		static ImGuiID dock_bottom;

		dock_left = ImGui::DockBuilderSplitNode(
			dock_main,
			ImGuiDir_Left,
			0.20f,
			nullptr,
			&dock_main);

		dock_right = ImGui::DockBuilderSplitNode(
			dock_main,
			ImGuiDir_Right,
			0.25f,
			nullptr,
			&dock_main);

		dock_bottom = ImGui::DockBuilderSplitNode(
			dock_main,
			ImGuiDir_Down,
			0.25f,
			nullptr,
			&dock_main);

		ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
		ImGui::DockBuilderDockWindow("Lights", dock_left);

		ImGui::DockBuilderDockWindow("Inspector", dock_right);
		ImGui::DockBuilderDockWindow("Graphics Settings", dock_right);

		ImGui::DockBuilderDockWindow("Assets", dock_bottom);

		ImGui::DockBuilderDockWindow("ViewPort", dock_main);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	if (ImGui::BeginMenuBar()) {

		if (ImGui::BeginMenu("File")) {

			if (ImGui::MenuItem("New")) {

				nfdu8char_t* outPath = nullptr;

				nfdfilteritem_t filterItem[1] = {
					{ "Text file", "bin" }
				};

				nfdresult_t result = NFD_SaveDialog(
					&outPath,
					filterItem,
					1,
					nullptr,
					nullptr
				);

				if (result == NFD_OKAY) {

					st.win_name = "void";
					st.win_mode = window_mode::WINDOWED;
					st.w = 1280;
					st.h = 720;
					st.VSync = 0;
					st.fov = 90;

					par.save("../data/settings.bin", outPath, st, cam, objects, lightObjects, lights);

					save_outPath = outPath;

					NFD_FreePath(outPath);
				}
			}

			if (ImGui::MenuItem("Load", "Ctrl+O")) {
				
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

					open_init("../data/settings.bin", path.c_str(), st);

					NFD_FreePath(outPath);
				}
			}

			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				
				par.skybox = st.skybox;

				cam.cam_position = cameraPos;

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

					par.save("../data/settings.bin", outPath, st, cam, objects, lightObjects, lights);

					save_outPath = outPath;

					NFD_FreePath(outPath);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Load_Camera")) {
				
				std::ifstream in("../data/settings.bin", std::ios::binary);

				par.load_settings(in, st, cam);
			}

			if (ImGui::MenuItem("Save_Camera")) {
				
				par.skybox = st.skybox;

				cam.cam_position = cameraPos;

				std::ofstream out("../data/settings.bin", std::ios::binary);

				par.save_settings(out, st, cam);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Load_Scene")) {
				
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

					load_scene_ok(outPath);

					NFD_FreePath(outPath);
				}
			}

			if (ImGui::MenuItem("Save_Scene")) {
				
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

					std::ofstream out(outPath, std::ios::binary);

					par.save_scene(out, objects, lightObjects, lights);

					save_outPath = outPath;

					NFD_FreePath(outPath);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit")) {
				
				quit = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {

			if (ImGui::MenuItem("Copy", "Ctrl+C")) {

				if (selectedObject != -1) {

					if (selectedObject < (int)objects.size()) {

						objCopy = objects[selectedObject];
						hasObjectCopy = true;
						hasLightCopy = false;

					}
					else {

						int lightIndex = selectedObject - (int)objects.size();

						objCopy = lightObjects[lightIndex];
						lightCopy = lights[lightIndex];

						hasObjectCopy = true;
						hasLightCopy = true;
					}
				}
			}

			if (ImGui::MenuItem("Paste", "Ctrl+V")) {

				if (hasObjectCopy) {

					if (hasLightCopy) {

						Object copy = objCopy;

						copy.vao = 0;
						copy.vbo = 0;
						copy.ebo = 0;

						copy.normalVAO = 0;
						copy.normalVBO = 0;

						copy.normalTBVAO = 0;
						copy.normalTBVBO = 0;

						copy.tangentVAO = 0;
						copy.tangentVBO = 0;

						copy.bitangentVAO = 0;
						copy.bitangentVBO = 0;

						buffer(copy);

						lightObjects.push_back(copy);
						lights.push_back(lightCopy);

						selectedObject = (int)(objects.size() + lightObjects.size() - 1);

					}
					else {

						Object copy = objCopy;

						copy.vao = 0;
						copy.vbo = 0;
						copy.ebo = 0;

						copy.normalVAO = 0;
						copy.normalVBO = 0;

						copy.normalTBVAO = 0;
						copy.normalTBVBO = 0;

						copy.tangentVAO = 0;
						copy.tangentVBO = 0;

						copy.bitangentVAO = 0;
						copy.bitangentVBO = 0;

						buffer(copy);

						objects.push_back(copy);
						selectedObject = (int)objects.size() - 1;
					}
				}
			}

			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
				
				if (selectedObject != -1) {

					if (selectedObject < objects.size()) {

						Object& obj = objects[selectedObject];

						if (!obj.undoStack.empty()) {

							obj.redoStack.push_back(CaptureState(obj));
							ApplyState(obj, obj.undoStack.back());
							obj.undoStack.pop_back();
							UpdateMapping(obj);
						}
					}
					else {

						int lightIndex = selectedObject - objects.size();

						Object& obj = lightObjects[lightIndex];
						Light& light = lights[lightIndex];

						if (!obj.undoStack.empty()) {

							obj.redoStack.push_back(CaptureState(obj));
							light.redoStack.push_back(CaptureLightState(light));

							ApplyState(obj, obj.undoStack.back());
							ApplyLightState(light, light.undoStack.back());

							obj.undoStack.pop_back();
							light.undoStack.pop_back();

							light.pos = obj.position;
							light.dir = glm::normalize(obj.rotation * glm::vec3(0.0f, -1.0f, 0.0f));

							obj.model = GetModelMatrix(obj);
							obj.mvp = projection * view * obj.model;
						}
					}
				}
			}

			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
				
				if (selectedObject != -1) {

					if (selectedObject < objects.size()) {

						Object& obj = objects[selectedObject];

						if (!obj.redoStack.empty()) {

							obj.undoStack.push_back(CaptureState(obj));
							ApplyState(obj, obj.redoStack.back());
							obj.redoStack.pop_back();
							UpdateMapping(obj);
						}
					}
					else {

						int lightIndex = selectedObject - objects.size();

						Object& obj = lightObjects[lightIndex];
						Light& light = lights[lightIndex];

						if (!obj.redoStack.empty()) {

							obj.undoStack.push_back(CaptureState(obj));
							light.undoStack.push_back(CaptureLightState(light));

							ApplyState(obj, obj.redoStack.back());
							ApplyLightState(light, light.redoStack.back());

							obj.redoStack.pop_back();
							light.redoStack.pop_back();

							light.pos = obj.position;
							light.dir = glm::normalize(obj.rotation * glm::vec3(0.0f, -1.0f, 0.0f));

							obj.model = GetModelMatrix(obj);
							obj.mvp = projection * view * obj.model;
						}
					}
				}
			}

			ImGui::EndMenu();
		}

		//if (ImGui::BeginMenu("View")) {

		//	ImGui::MenuItem("Hierarchy");
		//	ImGui::MenuItem("Assets");
		//	ImGui::MenuItem("Tools");
		//	ImGui::EndMenu();
		//}


		if (ImGui::BeginMenu("Settings")) {

			showGraphicsSettings = true;

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if (showGraphicsSettings) {

		ImGui::Begin("Graphics Settings", &showGraphicsSettings);

		ImGui::Checkbox("VSync", &st.VSync);

		if (!st.VSync)
			SDL_GL_SetSwapInterval(0);
		else
			SDL_GL_SetSwapInterval(1);

		ImGui::Separator();

		ImGui::Checkbox("Show Light", &st.light_active);

		ImGui::Checkbox("Show Shadows", &st.shadow_active);

		ImGui::Checkbox("Show Normal Map", &st.normal_map_active);

		ImGui::Checkbox("Show Wireframe", &st.wireframe_active);

		ImGui::Checkbox("Active global Physics", &st.physics_active);

		ImGui::Checkbox("Active global Collisions", &st.collision_active);

		if (ImGui::SliderFloat("FOV", &st.fov, 30.0f, 120.0f, "%.1f°")) {

			projection = glm::perspective(
				glm::radians(st.fov),
				(float)viewport_width / viewport_height,
				cam.nearPlane,
				cam.farPlane
			);
		}

		ImGui::End();
	}

	ImGui::End();
}

//--------------------------------------------------------------------------
// Viewport update size

void ResizeViewport(int width, int height) {

	camera_update = true;

	if (width <= 0 || height <= 0)
		return;

	viewport_width = width;
	viewport_height = height;

	glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

	glBindTexture(GL_TEXTURE_2D, viewport_texture);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB16F,
		width,
		height,
		0,
		GL_RGB,
		GL_FLOAT,
		nullptr
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		viewport_texture,
		0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = 0; i < MAX_LIGHTS; i++) {

		glDeleteFramebuffers(1, &depthFBO[i]);
	}

	for (int i = 0; i < MAX_LIGHTS; i++) {

		glGenFramebuffers(1, &depthFBO[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

		Create_depth_tex(viewport_width, viewport_height, i);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}

void ApplyWindowMode(SDL_Window* window) {

	SDL_DisplayID display = SDL_GetDisplayForWindow(window);

	SDL_Rect bounds;

	SDL_GetDisplayBounds(display, &bounds);

	switch (st.win_mode) {

		case window_mode::WINDOWED: {

			SDL_SetWindowFullscreen(window, false);

			SDL_SetWindowBordered(window, true);

			SDL_SetWindowSize(window,
				windowedSize.x,
				windowedSize.y);

			SDL_SetWindowPosition(
				window,
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED);

			st.w = windowedSize.x;
			st.h = windowedSize.y;

			break;
		}

		case window_mode::WINDOWED_BORDERLESS: {

			SDL_SetWindowFullscreen(window, false);

			SDL_SetWindowBordered(window, false);

			SDL_SetWindowPosition(window,
				bounds.x,
				bounds.y);

			SDL_SetWindowSize(window,
				bounds.w,
				bounds.h);

			st.w = bounds.w;
			st.h = bounds.h;

			break;
		}

		case window_mode::FULLSCREEN: {

			SDL_SetWindowFullscreen(window, true);

			SDL_GetWindowSizeInPixels(window,
				&st.w,
				&st.h);

			break;
		}
	}

	st.last_win_mode = st.win_mode;
}

//--------------------------------------------------------------------------
// Mapping

void FixCylinder(Object& obj) {

	for (size_t i = 0; i < obj.indices.size(); i += 3) {

		Vertex& v0 = obj.vertices[obj.indices[i + 0]];
		Vertex& v1 = obj.vertices[obj.indices[i + 1]];
		Vertex& v2 = obj.vertices[obj.indices[i + 2]];

		float u0 = v0.uv.x;
		float u1 = v1.uv.x;
		float u2 = v2.uv.x;

		float minU = std::min({ u0, u1, u2 });
		float maxU = std::max({ u0, u1, u2 });

		if (maxU - minU > 0.5f) {

			if (u0 < 0.5f) v0.uv.x += 1.0f;
			if (u1 < 0.5f) v1.uv.x += 1.0f;
			if (u2 < 0.5f) v2.uv.x += 1.0f;
		}
	}
}

void FixSphere(Object& obj) {

	std::vector<Vertex> newVertices = obj.vertices;
	std::vector<unsigned int> newIndices = obj.indices;

	for (size_t i = 0; i < newIndices.size(); i += 3) {

		unsigned int ids[3] = {
			newIndices[i + 0],
			newIndices[i + 1],
			newIndices[i + 2]
		};

		float u[3] = {
			newVertices[ids[0]].uv.x,
			newVertices[ids[1]].uv.x,
			newVertices[ids[2]].uv.x
		};

		float minU = std::min({ u[0], u[1], u[2] });
		float maxU = std::max({ u[0], u[1], u[2] });

		if (maxU - minU > 0.5f) {

			for (int k = 0; k < 3; ++k) {

				if (u[k] < 0.5f) {

					Vertex v = newVertices[ids[k]];
					v.uv.x += 1.0f;

					newVertices.push_back(v);
					newIndices[i + k] = (unsigned int)newVertices.size() - 1;
				}
			}
		}
	}

	obj.vertices = std::move(newVertices);
	obj.indices = std::move(newIndices);
}

void RestoreOriginalUV(Object& obj) {

	obj.vertices = obj.original_vertices;
	obj.indices = obj.original_indices;
}

void GeneratePlanarUV(Object& obj, coord_types coordType) {

	obj.vertices = obj.original_vertices;
	obj.indices = obj.original_indices;

	switch (coordType) {

		case coord_types::XZ: {

			float sizeX = obj.max.x - obj.min.x;
			float sizeZ = obj.max.z - obj.min.z;

			if (sizeX == 0.0f) sizeX = 1.0f;
			if (sizeZ == 0.0f) sizeZ = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.x - obj.min.x) / sizeX;
				vertex.uv.y = (vertex.position.z - obj.min.z) / sizeZ;
			}
			break;
		}

		case coord_types::XY: {

			float sizeX = obj.max.x - obj.min.x;
			float sizeY = obj.max.y - obj.min.y;

			if (sizeX == 0.0f) sizeX = 1.0f;
			if (sizeY == 0.0f) sizeY = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.x - obj.min.x) / sizeX;
				vertex.uv.y = (vertex.position.y - obj.min.y) / sizeY;
			}
			break;
		}

		case coord_types::YZ: {

			float sizeY = obj.max.y - obj.min.y;
			float sizeZ = obj.max.z - obj.min.z;

			if (sizeY == 0.0f) sizeY = 1.0f;
			if (sizeZ == 0.0f) sizeZ = 1.0f;

			for (auto& vertex : obj.vertices) {

				vertex.uv.x = (vertex.position.y - obj.min.y) / sizeY;
				vertex.uv.y = (vertex.position.z - obj.min.z) / sizeZ;
			}
			break;
		}
	}
}

void GenerateCubeUV(Object& obj) {

	obj.vertices = obj.original_vertices;
	obj.indices = obj.original_indices;

	for (auto& v : obj.vertices) {
	
		glm::vec3 n = glm::normalize(v.normal);

		float ax = fabs(n.x);
		float ay = fabs(n.y);
		float az = fabs(n.z);

		if (ax >= ay && ax >= az) {

			if (n.x > 0) {

				v.uv.x = (-v.position.z - obj.min.z) / (obj.max.z - obj.min.z);

				v.uv.y = (v.position.y - obj.min.y) / (obj.max.y - obj.min.y);
			}else {

				v.uv.x = (v.position.z - obj.min.z) / (obj.max.z - obj.min.z);

				v.uv.y = (v.position.y - obj.min.y) / (obj.max.y - obj.min.y);
			}
		}else if (ay >= ax && ay >= az) {

			if (n.y > 0) {

				v.uv.x = (v.position.x - obj.min.x) / (obj.max.x - obj.min.x);

				v.uv.y = (-v.position.z - obj.min.z) / (obj.max.z - obj.min.z);
			}else {

				v.uv.x = (v.position.x - obj.min.x) / (obj.max.x - obj.min.x);

				v.uv.y = (v.position.z - obj.min.z) / (obj.max.z - obj.min.z);
			}
		}else {

			if (n.z > 0) {

				v.uv.x = (v.position.x - obj.min.x) / (obj.max.x - obj.min.x);

				v.uv.y = (v.position.y - obj.min.y) / (obj.max.y - obj.min.y);
			}
			else {

				v.uv.x = (-v.position.x - obj.min.x) / (obj.max.x - obj.min.x);

				v.uv.y = (v.position.y - obj.min.y) / (obj.max.y - obj.min.y);
			}
		}
	}
}

void GenerateCylindricalUV(Object& obj, cylinder_axis axis) {

	obj.vertices = obj.original_vertices;
	obj.indices = obj.original_indices;

	static float doble_pi = 2.0f * glm::pi<float>();

	switch (axis) {
	
		case cylinder_axis::X: {
	
			float size = obj.max.x - obj.min.x;
			if (size == 0.0f) 
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.y, vertex.position.z);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.x - obj.min.x) / size;
			}

			break;
		}

		case cylinder_axis::Y: {

			float size = obj.max.y - obj.min.y;
			if (size == 0.0f) 
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.z, vertex.position.x);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.y - obj.min.y) / size;
			}

			break;
		}

		case cylinder_axis::Z: {

			float size = obj.max.z - obj.min.z;
			if (size == 0.0f)
				size = 1.0f;

			for (auto& vertex : obj.vertices) {

				float angle = atan2f(vertex.position.y, vertex.position.x);

				float u = (angle + glm::pi<float>()) / (doble_pi);
				vertex.uv.x = 1.0f - u;
				vertex.uv.y = (vertex.position.z - obj.min.z) / size;
			}

			break;
		}
	}

	FixCylinder(obj);
}

void GenerateSphericalUV(Object& obj) {

	obj.vertices = obj.original_vertices;
	obj.indices = obj.original_indices;

	constexpr float TWO_PI = glm::two_pi<float>();

	for (auto& v : obj.vertices) {

		glm::vec3 p = glm::normalize(v.position);

		float u = atan2(p.z, p.x);
		float vv = asin(p.y);

		v.uv.x = (u + glm::pi<float>()) / TWO_PI;
		v.uv.y = (vv + glm::half_pi<float>()) / glm::pi<float>();
	}

	FixSphere(obj);
}

void UpdateMapping(Object& obj) {

	switch (obj.mapping) {

	case TextureMapping::UV:
		RestoreOriginalUV(obj);
		break;

	case TextureMapping::Planar:
		GeneratePlanarUV(obj, obj.coordType);
		break;

	case TextureMapping::Cubic:
		GenerateCubeUV(obj);
		break;

	case TextureMapping::Cylindrical:
		GenerateCylindricalUV(obj, obj.cyaxisType);
		break;

	case TextureMapping::Spherical:
		GenerateSphericalUV(obj);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);

	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);

	glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(Vertex), obj.vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), obj.indices.data(), GL_STATIC_DRAW);
}

//--------------------------------------------------------------------------
// Reload folder

void ReloadFolder() {

	folderEntries.clear();

	for (auto& e : std::filesystem::directory_iterator(currentFolder))
		folderEntries.push_back(e);
}

//--------------------------------------------------------------------------
// Obj names

std::string GenerateObjectName(const std::string& baseName) {

	int& counter = objectNameCounters[baseName];

	if (counter == 0) {

		counter = 1;
		return baseName;
	}

	return baseName + "_" + std::to_string(counter++);
}

//--------------------------------------------------------------------------
// Reload textures path

void read_textures_path() {

	pathTextures.clear();

	for (const auto& entry : std::filesystem::directory_iterator("../" + dirPath)) {

		if (!entry.is_regular_file())
			continue;

		pathTextures.push_back(entry.path());
	}
}

//--------------------------------------------------------------------------
// Skybox and cubemap

void gen_cubemap() {

	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	for (int i = 0; i < 6; i++) {

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

	glRenderbufferStorage(
		GL_RENDERBUFFER,
		GL_DEPTH_COMPONENT24,
		1024,
		1024
	);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		captureRBO
	);
}

void ConvertHDRToCubemap() {

	glViewport(0, 0, 1024, 1024);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glUseProgram(program_hdrToCubemap);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skybox_texture);

	glUniform1i(
		glGetUniformLocation(program_hdrToCubemap,
			"equirectangularMap"),
		0);

	glUniformMatrix4fv(
		glGetUniformLocation(program_hdrToCubemap, "projection"),
		1,
		GL_FALSE,
		glm::value_ptr(captureProjection));

	glBindVertexArray(skyboxVAO);

	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		captureRBO);

	for (int i = 0; i < 6; i++) {

		glUniformMatrix4fv(
			glGetUniformLocation(program_hdrToCubemap, "view"),
			1,
			GL_FALSE,
			glm::value_ptr(captureViews[i]));

		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			envCubemap,
			0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, st.w, st.h);
}

void GenSkybox() {

	// my friend LearnOpenGL carries
	float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);

	glBindVertexArray(skyboxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

//--------------------------------------------------------------------------
// Getters

glm::vec3 get_cam_pos() {

	glm::vec3 pos;

	pos.x = cam.radius * sin(cam.beta) * cos(cam.alpha);
	pos.y = cam.radius * cos(cam.beta);
	pos.z = cam.radius * sin(cam.beta) * sin(cam.alpha);

	return cam.cam_target + pos;
}

Object* GetSelectedObject() {

	if (selectedObject < 0)
		return nullptr;

	if (selectedObject < objects.size())
		return &objects[selectedObject];

	int lightIndex = selectedObject - objects.size();

	if (lightIndex >= lightObjects.size())
		return nullptr;

	return &lightObjects[lightIndex];
}

Light* GetSelectedLight() {

	if (selectedObject < objects.size())
		return nullptr;

	int lightIndex = selectedObject - objects.size();

	if (lightIndex >= lights.size())
		return nullptr;

	return &lights[lightIndex];
}

glm::mat4 GetModelMatrix(Object& obj) {

	glm::mat4 m(1);

	m = glm::translate(m, obj.position);

	m *= glm::toMat4(obj.rotation);

	m = glm::scale(m, obj.scale);

	return m;
}

GLuint TextureManager::Get(const std::string& path) {

	auto it = textures.find(path);

	if (it != textures.end())
		return it->second;

	GLuint texture = LoadObjTexture(path);

	if (texture != 0)
		textures[path] = texture;

	return texture;
}

void TextureManager::Clear() {

	for (auto& [path, id] : textures) {

		glDeleteTextures(1, &id);
	}

	textures.clear();
}

//--------------------------------------------------------------------------
// Ray-AABB intersection test

bool RayVsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t) {

	float tmin = 0.0f;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; i++) {

		if (fabs(rayDir[i]) < 0.000001f) {

			if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i])
				return false;
		}
		else {

			float inv = 1.0f / rayDir[i];

			float t1 = (boxMin[i] - rayOrigin[i]) * inv;
			float t2 = (boxMax[i] - rayOrigin[i]) * inv;

			if (t1 > t2)
				std::swap(t1, t2);

			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);

			if (tmin > tmax)
				return false;
		}
	}

	t = tmin;

	return true;
}

void BuildRay(float mouseX, float mouseY, glm::vec3& rayOrigin, glm::vec3& rayDir) {

	float x = (2.0f * mouseX) / viewport_width - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / viewport_height;

	glm::vec4 rayClip(x, y, -1.0f, 1.0f);

	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::vec4 rayWorld = glm::inverse(view) * rayEye;

	rayDir = glm::normalize(glm::vec3(rayWorld));

	rayOrigin = get_cam_pos();
}

//--------------------------------------------------------------------------
// Select obj

void select_object(glm::vec3& rayOrigin, glm::vec3& rayDir) {

	float nearest = FLT_MAX;

	selectedObject = -1;

	for (size_t i = 0; i < objects.size(); i++) {

		Object& obj = objects[i];

		glm::vec3 worldMin = obj.min * obj.scale + obj.position;
		glm::vec3 worldMax = obj.max * obj.scale + obj.position;

		float t;

		if (RayVsAABB(rayOrigin, rayDir, worldMin, worldMax, t)) {

			if (t < nearest) {

				nearest = t;
				selectedObject = static_cast<int>(i);
			}
		}
	}

	for (size_t i = 0; i < lightObjects.size(); i++) {

		Object& obj = lightObjects[i];

		glm::vec3 worldMin = obj.min * obj.scale + obj.position;
		glm::vec3 worldMax = obj.max * obj.scale + obj.position;

		float t;

		if (RayVsAABB(rayOrigin, rayDir, worldMin, worldMax, t)) {

			if (t < nearest) {

				nearest = t;
				selectedObject = static_cast<int>(objects.size() + i);
			}
		}
	}
}

//--------------------------------------------------------------------------
// Controls

void controls(const bool* keyboard, float angle_step, float radius_step, float dt) {

	if (currentViewMode == view_Mode::EDITOR) {

		glm::vec3 cameraPos = get_cam_pos();

		glm::vec3 forward = glm::normalize(cam.cam_target - cameraPos);

		forward.y = 0.0f;

		if (glm::length2(forward) > 0.0001f)
			forward = glm::normalize(forward);

		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

		float speed = moveSpeed * (cam.radius * 0.09f);

		if (keyboard[SDL_SCANCODE_W])
			cam.cam_target += forward * speed * dt;

		if (keyboard[SDL_SCANCODE_S])
			cam.cam_target -= forward * speed * dt;

		if (keyboard[SDL_SCANCODE_A])
			cam.cam_target -= right * speed * dt;

		if (keyboard[SDL_SCANCODE_D])
			cam.cam_target += right * speed * dt;

		cam.beta = glm::clamp(cam.beta, 0.01f, glm::pi<float>() - 0.01f);

		cam.radius = std::max(cam.radius, 0.5f);

		return;
	}
}

//--------------------------------------------------------------------------
// Grid

void Create_Grid() {

	std::vector<glm::vec3> vertices;

	const int SIZE = 1000;

	for (int i = -SIZE; i <= SIZE; i++) {

		vertices.push_back(glm::vec3(i * STEP, 0.0f, -SIZE * STEP));
		vertices.push_back(glm::vec3(i * STEP, 0.0f, SIZE * STEP));

		vertices.push_back(glm::vec3(-SIZE * STEP, 0.0f, i * STEP));
		vertices.push_back(glm::vec3(SIZE * STEP, 0.0f, i * STEP));
	}

	gridVertexCount = vertices.size();

	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &gridVBO);

	glBindVertexArray(gridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

	glBufferData(
		GL_ARRAY_BUFFER,
		vertices.size() * sizeof(glm::vec3),
		vertices.data(),
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(glm::vec3),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

//--------------------------------------------------------------------------
// Lines

void Build_Normals(Object& obj) {

	obj.normalLines.clear();

	float length = 0.25f;

	for (auto& v : obj.vertices) {

		glm::vec3 p = v.position;
		glm::vec3 n = glm::normalize(v.normal);

		obj.normalLines.push_back({ p });
		obj.normalLines.push_back({ p + n * length });
	}
}

void Build_TBN(Object& obj) {

	obj.normalLinesTBN.clear();
	obj.tangentLines.clear();
	obj.bitangentLines.clear();

	float length = 0.25f;

	for (auto& v : obj.vertices) {

		glm::vec3 p = v.position;

		glm::vec3 N = glm::normalize(v.normal);
		glm::vec3 T = glm::normalize(v.tangent);

		T = glm::normalize(T - N * glm::dot(N, T));

		glm::vec3 B = -glm::normalize(glm::cross(N, T));

		obj.normalLinesTBN.push_back({ p });
		obj.normalLinesTBN.push_back({ p + N * length });

		obj.tangentLines.push_back({ p });
		obj.tangentLines.push_back({ p + T * length });

		obj.bitangentLines.push_back({ p });
		obj.bitangentLines.push_back({ p + B * length });
	}
}

void CreateLineBuffer(GLuint& vao, GLuint& vbo, const std::vector<LineVertex>& data) {

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(LineVertex)), data.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);

	glBindVertexArray(0);
}

void Calculate_Average_Normals(Object& obj) {

	float eps = 0.0001f;

	std::vector<glm::vec3> original_normals(obj.vertices.size());

	for (int i = 0; i < obj.vertices.size(); i++)
		original_normals[i] = obj.vertices[i].normal;

	for (int i = 0; i < obj.vertices.size(); i++) {

		glm::vec3 avg(0.0f);
		int count = 0;

		for (int j = 0; j < obj.vertices.size(); j++) {

			if (glm::length(obj.vertices[i].position - obj.vertices[j].position) < eps) {

				avg += original_normals[j];
				count++;
			}
		}

		if (count > 0)
			obj.vertices[i].normal = glm::normalize(avg);
	}
}

void Calculate_Average_TBN(Object& obj) {

	const float posEps = 0.0001f;
	const float uvEps = 0.0001f;

	std::vector<glm::vec3> normals(obj.vertices.size());
	std::vector<glm::vec3> tangents(obj.vertices.size());

	for (size_t i = 0; i < obj.vertices.size(); i++) {

		normals[i] = obj.vertices[i].normal;
		tangents[i] = obj.vertices[i].tangent;
	}

	for (size_t i = 0; i < obj.vertices.size(); i++) {

		glm::vec3 avgN(0.0f);
		glm::vec3 avgT(0.0f);

		int count = 0;

		for (size_t j = 0; j < obj.vertices.size(); j++) {

			bool samePos =
				glm::length(obj.vertices[i].position - obj.vertices[j].position) < posEps;

			bool sameUV =
				glm::length(obj.vertices[i].uv - obj.vertices[j].uv) < uvEps;

			if (samePos && sameUV) {

				avgN += normals[j];
				avgT += tangents[j];
				count++;
			}
		}

		if (count == 0)
			continue;

		avgN = glm::normalize(avgN);

		avgT = avgT - avgN * glm::dot(avgN, avgT);

		if (glm::length2(avgT) < 1e-8f){

			if (fabs(avgN.y) < 0.999f)
				avgT = glm::normalize(glm::cross(glm::vec3(0, 1, 0), avgN));
			else
				avgT = glm::normalize(glm::cross(glm::vec3(1, 0, 0), avgN));
		}
		else
			avgT = glm::normalize(avgT);
		

		glm::vec3 avgB = glm::normalize(glm::cross(avgN, avgT));

		obj.vertices[i].normal = avgN;
		obj.vertices[i].tangent = avgT;
		obj.vertices[i].bitangent = avgB;
	}
}

glm::vec3 Compute_Normal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
	return glm::normalize(glm::cross(b - a, c - a));
}

//--------------------------------------------------------------------------
// Vertex and triangle helpers

void Add_Vertex(Object& obj, const Vertex& v) {

	obj.vertices.push_back(v);
	obj.indices.push_back(obj.indices.size());
}

void Add_Vertex(Light& lgh, const Vertex& v) {

	lgh.vertices.push_back(v);
	lgh.indices.push_back(lgh.indices.size());
}

void Add_Tri(Object& obj, const Vertex& in_v0, const Vertex& in_v1, const Vertex& in_v2) {

	// the reason for this is that i need to mantain the const, because in gen_figures.h i call this func directly doing Vertex{}, so i must take const
	// but, to avoid doing a const_cast, i prefer doing this
	Vertex v0 = in_v0;
	Vertex v1 = in_v1;
	Vertex v2 = in_v2;

	glm::vec3 faceN = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));

	glm::vec3 delta_v1 = v1.position - v0.position;
	glm::vec3 delta_v2 = v2.position - v0.position;

	glm::vec2 delta_uv1 = v1.uv - v0.uv;
	glm::vec2 delta_uv2 = v2.uv - v0.uv;

	float det = delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y;

	if (fabs(det) < 1e-8f)
		return;

	float f = 1.0f / det;

	glm::vec3 T = f * (delta_uv2.y * delta_v1 - delta_uv1.y * delta_v2);

	glm::vec3 B = f * (-delta_uv2.x * delta_v1 + delta_uv1.x * delta_v2);

	glm::vec3 N = faceN;

	v0.tangent += T;
	v1.tangent += T;
	v2.tangent += T;

	v0.bitangent += B;
	v1.bitangent += B;
	v2.bitangent += B;

	v0.normal += N;
	v1.normal += N;
	v2.normal += N;

	Add_Vertex(obj, v0);
	Add_Vertex(obj, v1);
	Add_Vertex(obj, v2);
}

void Add_Tri(Light& lgh, const  Vertex& in_v0, const  Vertex& in_v1, const  Vertex& in_v2) {

	// the reason for this is that i need to mantain the const, because in gen_figures.h i call this func directly doing Vertex{}, so i must take const
	// but, to avoid doing a const_cast, i prefer doing this
	Vertex v0 = in_v0;
	Vertex v1 = in_v1;
	Vertex v2 = in_v2;

	glm::vec3 faceN = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));

	v0.tangent = glm::vec3(0);
	v1.tangent = glm::vec3(0);
	v2.tangent = glm::vec3(0);

	v0.bitangent = glm::vec3(0);
	v1.bitangent = glm::vec3(0);
	v2.bitangent = glm::vec3(0);

	Add_Vertex(lgh, v0);
	Add_Vertex(lgh, v1);
	Add_Vertex(lgh, v2);
}

//--------------------------------------------------------------------------
// Create Lights

void CreateLight(const std::string& type) {

	Light light;
	Object obj;

	if (!init_lights(obj))
		return;

	light.name = GenerateObjectName("Light");
	obj.name = GenerateObjectName("Light");

	light.pos = glm::vec3(100.0f);
	light.dir = glm::vec3(0.0f, -1.0f, 0.0f);

	light.col = glm::vec3(1.0f);

	light.att = glm::vec3(1.0f, 0.0005f, 0.00001f);

	light.ambient = 0.15f;

	light.bias = 0.000008f;
	light.pcf = 3.0f;

	light.inner = 10.f;
	light.outer = 40.f;
	light.falloff = 1.f;

	light.type = type;

	if (type == "POINT")
		light.typeID = 0;
	else if (type == "DIR")
		light.typeID = 1;
	else
		light.typeID = 2;

	obj.position = light.pos;
	obj.rotation = glm::identity<glm::quat>();
	obj.scale = glm::vec3(20.f);

	obj.model = GetModelMatrix(obj);

	buffer(obj);

	lights.push_back(light);
	lightObjects.push_back(obj);
}

//--------------------------------------------------------------------------
// Clean buffers and objects

void clean() {

	glDeleteProgram(program);
	glDeleteProgram(program_line);
	glDeleteProgram(program_grid);
	glDeleteProgram(program_depth);
	glDeleteProgram(program_skybox);
	glDeleteProgram(program_hdrToCubemap);

	glDeleteVertexArrays(1, &gridVAO);
	glDeleteVertexArrays(1, &skyVAO);
	glDeleteVertexArrays(1, &skyboxVAO);

	glDeleteBuffers(1, &gridVBO);
	glDeleteBuffers(1, &skyVBO);
	glDeleteBuffers(1, &skyboxVBO);

	glDeleteTextures(1, &envCubemap);
	glDeleteTextures(1, &dynamic_cubemap);
	glDeleteTextures(1, &viewport_texture);

	glDeleteFramebuffers(1, &viewportFBO);
	glDeleteFramebuffers(1, &captureFBO);
	glDeleteFramebuffers(1, &captureRBO);

	for (int i = 0; i < MAX_LIGHTS; i++)
		glDeleteTextures(1, &depth_texture[i]);

	glDeleteTextures(1, &skybox_texture);

	for (auto& obj : objects) {

		DestroyBuffers(obj);

		if (obj.normalTexture != 0)
			glDeleteTextures(1, &obj.normalTexture);

		obj.vertices.clear();
		obj.indices.clear();

		obj.original_vertices.clear();
		obj.original_indices.clear();

		obj.normalLines.clear();
		obj.bitangentLines.clear();
		obj.tangentLines.clear();

		obj.normalLinesTBN.clear();

		obj.undoStack.clear();
		obj.redoStack.clear();
	}

	for (auto& obj : lightObjects) {

		DestroyBuffers(obj);

		if (obj.normalTexture != 0)
			glDeleteTextures(1, &obj.normalTexture);

		obj.vertices.clear();
		obj.indices.clear();

		obj.original_vertices.clear();
		obj.original_indices.clear();

		obj.normalLines.clear();
		obj.bitangentLines.clear();
		obj.tangentLines.clear();

		obj.normalLinesTBN.clear();

		obj.undoStack.clear();
		obj.redoStack.clear();
	}

	objects.clear();
	lightObjects.clear();
	lights.clear();

	objectNameCounters.clear();

	pathTextures.clear();

	folderEntries.clear();

	TextureManager::Clear();
}

void DestroyBuffers(Object& obj) {

	if (obj.vao) {
	
		glDeleteVertexArrays(1, &obj.vao);
		obj.vao = 0;
	}

	if (obj.vbo) {
	
		glDeleteBuffers(1, &obj.vbo);
		obj.vbo = 0;
	}

	if (obj.ebo) {
	
		glDeleteBuffers(1, &obj.ebo);
		obj.ebo = 0;
	}

	if (obj.normalVAO) {
	
		glDeleteVertexArrays(1, &obj.normalVAO);
		obj.normalVAO = 0;
	}

	if (obj.normalVBO) {
	
		glDeleteBuffers(1, &obj.normalVBO);
		obj.normalVBO = 0;
	}

	if (obj.normalTBVAO) {
	
		glDeleteVertexArrays(1, &obj.normalTBVAO);
		obj.normalTBVAO = 0;
	}

	if (obj.normalTBVBO) {
	
		glDeleteBuffers(1, &obj.normalTBVBO);
		obj.normalTBVBO = 0;
	}

	if (obj.tangentVAO) {
	
		glDeleteVertexArrays(1, &obj.tangentVAO);
		obj.tangentVAO = 0;
	}

	if (obj.tangentVBO) {
	
		glDeleteBuffers(1, &obj.tangentVBO);
		obj.tangentVBO = 0;
	}

	if (obj.bitangentVAO) {
	
		glDeleteVertexArrays(1, &obj.bitangentVAO);
		obj.bitangentVAO = 0;
	}

	if (obj.bitangentVBO) {
	
		glDeleteBuffers(1, &obj.bitangentVBO);
		obj.bitangentVBO = 0;
	}
}

//--------------------------------------------------------------------------
// Mesh replacement

void ReplaceMesh(Object& obj, const std::string& path) {

	Object mesh;

	init_obj(path.c_str(), mesh);
	buffer(mesh);

	mesh.position = obj.position;
	mesh.rotation = obj.rotation;
	mesh.scale = obj.scale;

	mesh.original_vertices = mesh.vertices;
	mesh.original_indices = mesh.indices;

	mesh.obj_texture = obj.obj_texture;
	mesh.tex = obj.tex;

	mesh.normalTexture = obj.normalTexture;
	mesh.norm_map = obj.norm_map;

	mesh.has_normal_map = obj.has_normal_map;

	mesh.shininess = obj.shininess;

	mesh.physics = obj.physics;
	mesh.hasPhysics = obj.hasPhysics;

	mesh.collider = obj.collider;
	mesh.hasCollider = obj.hasCollider;

	mesh.visible = obj.visible;

	mesh.name = obj.name;
	mesh.mesh = path;

	DestroyBuffers(obj);

	obj = std::move(mesh);
}

//--------------------------------------------------------------------------
// Undo/Redo

void HandleInspectorUndo(Object& obj) {

	static ObjectState state;

	if (ImGui::IsItemActivated())
		state = CaptureState(obj);

	if (ImGui::IsItemDeactivatedAfterEdit()) {

		obj.undoStack.push_back(state);
		obj.redoStack.clear();
	}
}

void HandleInspectorUndo(Light& light, Object& lightObj) {

	if (ImGui::IsItemActivated()) {
		prevLightState = CaptureLightState(light);
		prevState = CaptureState(lightObj);
	}

	if (ImGui::IsItemDeactivatedAfterEdit()) {
		light.undoStack.push_back(prevLightState);
		light.redoStack.clear();

		lightObj.undoStack.push_back(prevState);
		lightObj.redoStack.clear();
	}
}

//--------------------------------------------------------------------------
// Physics

void AddForce(Object& obj, const glm::vec3& force) {

	obj.physics.force += force;
}

//--------------------------------------------------------------------------
// Collisions

bool CanCollide(const Object& a, const Object& b) {

	return (a.collider.mask & b.collider.layer) && (b.collider.mask & a.collider.layer);
}

Contact Sphere_vs_Sphere(const Object& a, const Object& b) {

	Contact contact{};

	glm::vec3 delta = b.position - a.position;

	float dist2 = glm::dot(delta, delta);

	float radiusA = GetWorldRadius(a);
	float radiusB = GetWorldRadius(b);

	float radiusSum = radiusA + radiusB;

	if (dist2 >= radiusSum * radiusSum)
		return contact;

	contact.hit = true;

	float dist = sqrt(dist2);

	if (dist < 1e-6f) {

		contact.normal = glm::vec3(1, 0, 0);
		contact.penetration = radiusSum;
		contact.point = a.position;

	} else {

		contact.normal = delta / dist;
		contact.penetration = radiusSum - dist;
		contact.point = a.position + contact.normal * radiusA;
	}

	return contact;
}

Contact Sphere_vs_Plane(const Object& sphere, const Object& plane) {

	Contact contact{};

	glm::quat invRotation = glm::inverse(plane.rotation);

	glm::vec3 localCenter = invRotation * (sphere.position - plane.position);

	float radius = GetWorldRadius(sphere);

	float halfWidth = std::abs(plane.scale.x);
	float halfDepth = std::abs(plane.scale.z);

	float distance = localCenter.y;

	if (std::abs(distance) > radius)
		return contact;

	// this clamps is to check if local center is into de xz bounds of the plane
	float closestX = glm::clamp(localCenter.x, -halfWidth, halfWidth);

	float closestZ = glm::clamp(localCenter.z, -halfDepth, halfDepth);

	// calc the distance between sphere center and closest point to the plane
	glm::vec3 closestPointLocal(closestX, 0.0f, closestZ);

	glm::vec3 deltaLocal = localCenter - closestPointLocal;

	float distance2 = glm::dot(deltaLocal, deltaLocal);

	if (distance2 > radius * radius)
		return contact;

	float distanceToSurface = std::sqrt(distance2);

	glm::vec3 normalLocal;

	if (distanceToSurface > 1e-6f) {

		normalLocal = deltaLocal / distanceToSurface;

	} else
		normalLocal = glm::vec3(0, 1, 0);

	glm::vec3 worldNormal = plane.rotation * normalLocal;

	worldNormal = glm::normalize(worldNormal);

	contact.hit = true;

	contact.normal = (distance >= 0.0f) ? -worldNormal : worldNormal;

	contact.penetration =
		radius - distanceToSurface;

	contact.point =
		sphere.position + contact.normal * radius;

	return contact;
}

Contact Sphere_vs_AABB(const Object& sphere, const Object& box) {

	Contact contact{};

	glm::vec3 half = glm::abs(box.scale);

	glm::vec3 min = box.position - half;
	glm::vec3 max = box.position + half;

	glm::vec3 closest = glm::clamp(sphere.position, min, max);

	glm::vec3 delta = closest - sphere.position;

	float dist2 = glm::dot(delta, delta);

	float radius = GetWorldRadius(sphere);

	if (dist2 > radius * radius)
		return contact;

	contact.hit = true;

	float dist = sqrt(dist2);

	if (dist > 1e-6f) {

		contact.normal = glm::normalize(delta);
		contact.penetration = radius - dist;
		contact.point = closest;

	} else {

		contact.normal = glm::vec3(1, 0, 0);
		contact.penetration = radius;
		contact.point = sphere.position;
	}

	return contact;
}

Contact AABB_vs_AABB(const Object& a, const Object& b) {

	Contact contact{};

	glm::vec3 halfA = glm::abs(a.scale);
	glm::vec3 halfB = glm::abs(b.scale);

	glm::vec3 delta = b.position - a.position;

	float overlapX = halfA.x + halfB.x - std::abs(delta.x);
	if (overlapX <= 0.0f)
		return contact;

	float overlapY = halfA.y + halfB.y - std::abs(delta.y);
	if (overlapY <= 0.0f)
		return contact;

	float overlapZ = halfA.z + halfB.z - std::abs(delta.z);
	if (overlapZ <= 0.0f)
		return contact;

	contact.hit = true;

	if (overlapX < overlapY && overlapX < overlapZ) {

		contact.penetration = overlapX;
		contact.normal = glm::vec3(delta.x > 0 ? 1.f : -1.f, 0, 0);

	} else if (overlapY < overlapZ) {

		contact.penetration = overlapY;
		contact.normal = glm::vec3(0, delta.y > 0 ? 1.f : -1.f, 0);

	} else {

		contact.penetration = overlapZ;
		contact.normal = glm::vec3(0, 0, delta.z > 0 ? 1.f : -1.f);
	}

	contact.point = (a.position + b.position) * 0.5f;

	return contact;
}

Contact AABB_vs_Plane(const Object& box, const Object& plane) {

	Contact contact{};

	glm::quat invRotation = glm::inverse(plane.rotation);

	glm::vec3 localCenter = invRotation * (box.position - plane.position);

	glm::vec3 half = glm::abs(box.scale);

	float halfWidth = std::abs(plane.scale.x);
	float halfDepth = std::abs(plane.scale.z);

	glm::vec3 closestPointLocal = localCenter;

	closestPointLocal.x = glm::clamp(localCenter.x, -halfWidth, halfWidth);
	closestPointLocal.z = glm::clamp(localCenter.z, -halfDepth, halfDepth);

	if (std::abs(localCenter.x) > halfWidth + half.x)
		return contact;

	if (std::abs(localCenter.z) > halfDepth + half.z)
		return contact;

	float distance = localCenter.y;

	float minY = localCenter.y - half.y;
	float maxY = localCenter.y + half.y;

	if (minY > 0.0f || maxY < 0.0f)
		return contact;

	glm::vec3 normalLocal = (distance >= 0.0f) ? glm::vec3(0, -1, 0) : glm::vec3(0, 1, 0);

	glm::vec3 worldNormal = glm::normalize(plane.rotation * normalLocal);

	contact.hit = true;
	contact.normal = worldNormal;
	
	if (localCenter.y >= 0.0f)
		contact.penetration = half.y - localCenter.y;
	else
		contact.penetration = half.y + localCenter.y;

	glm::vec3 worldPoint = plane.position + plane.rotation * closestPointLocal;

	contact.point = worldPoint;

	return contact;
}

void PositionalCorrection(Object& a, Object& b, Contact& c) {

	Physics& pa = a.physics;
	Physics& pb = b.physics;

	if (pa.isStatic && pb.isStatic)
		return;

	float invMassA = pa.inverseMass;
	float invMassB = pb.inverseMass;

	float totalInvMass = invMassA + invMassB;

	if (totalInvMass == 0.0f)
		return;

	static const float percent = 0.8f;
	static const float slop = 0.001f;

	glm::vec3 correction = c.normal * percent * std::max(c.penetration - slop, 0.0f) / totalInvMass;

	if (!pa.isStatic)
		a.position -= correction * invMassA;

	if (!pb.isStatic)
		b.position += correction * invMassB;
}

void ResolveVelocity(Object& a, Object& b, Contact& c) {

	Physics& pa = a.physics;
	Physics& pb = b.physics;

	if (pa.isStatic && pb.isStatic)
		return;

	float invMassA = pa.inverseMass;
	float invMassB = pb.inverseMass;

	float totalInvMass = invMassA + invMassB;

	if (totalInvMass == 0.0f)
		return;

	glm::vec3 relativeVelocity = pb.velocity - pa.velocity;

	float velAlongNormal = glm::dot(relativeVelocity, c.normal);

	if (velAlongNormal > 0.0f)
		return;

	float e = std::min(pa.restitution, pb.restitution);

	float j = -(1.0f + e) * velAlongNormal;
	j /= totalInvMass;

	glm::vec3 impulse = j * c.normal;

	if (!pa.isStatic)
		pa.velocity -= impulse * invMassA;

	if (!pb.isStatic)
		pb.velocity += impulse * invMassB;
}

float GetWorldRadius(const Object& obj) {

	float s = glm::compMax(glm::abs(obj.scale));
	return obj.collider.radius * s;
}

//--------------------------------------------------------------------------

camera c;

void save_scene_state() {

	for (auto& obj : saved_objects)
		DestroyBuffers(obj);

	for (auto& obj : saved_light_objects)
		DestroyBuffers(obj);

	saved_objects.clear();
	saved_light_objects.clear();
	saved_lights.clear();

	saved_objects = objects;

	for (auto& obj : saved_objects) {

		obj.vao = 0;
		obj.vbo = 0;
		obj.ebo = 0;

		obj.normalVAO = 0;
		obj.normalVBO = 0;

		obj.normalTBVAO = 0;
		obj.normalTBVBO = 0;

		obj.tangentVAO = 0;
		obj.tangentVBO = 0;

		obj.bitangentVAO = 0;
		obj.bitangentVBO = 0;
	}

	saved_light_objects = lightObjects;

	for (auto& obj : saved_light_objects) {

		obj.vao = 0;
		obj.vbo = 0;
		obj.ebo = 0;

		obj.normalVAO = 0;
		obj.normalVBO = 0;

		obj.normalTBVAO = 0;
		obj.normalTBVBO = 0;

		obj.tangentVAO = 0;
		obj.tangentVBO = 0;

		obj.bitangentVAO = 0;
		obj.bitangentVBO = 0;
	}

	saved_lights = lights;

	c = cam;
}

void load_scene_state() {

	for (auto& obj : objects)
		DestroyBuffers(obj);

	for (auto& obj : lightObjects)
		DestroyBuffers(obj);

	objects = saved_objects;
	lightObjects = saved_light_objects;
	lights = saved_lights;

	for (auto& obj : objects)
		buffer(obj);

	for (auto& obj : lightObjects)
		buffer(obj);

	cam = c;
}

Object* search_obj_by_name(const char* name) {

	for (auto& obj : objects) {
		
		if (obj.name == name) {
		
			return &obj;
		}
	}

	return nullptr;
}

void PlayerControl(const bool* keyboard, float dt, Object& player) {

	// PLAY
	// ---------------------------------------------------------

	glm::vec3 forward(cos(cam.alpha), 0.0f, sin(cam.alpha));

	forward = glm::normalize(forward);

	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

	glm::vec3 moveDirection(0.0f);

	if (keyboard[SDL_SCANCODE_W])
		moveDirection += forward;

	if (keyboard[SDL_SCANCODE_S])
		moveDirection -= forward;

	if (keyboard[SDL_SCANCODE_A])
		moveDirection -= right;

	if (keyboard[SDL_SCANCODE_D])
		moveDirection += right;

	if (glm::length2(moveDirection) > 0.0001f)
		moveDirection = glm::normalize(moveDirection);

	float moveForce = 100.0f;

	player.physics.force += moveDirection * moveForce;
}

void UpdatePlayerCamera(Object& player) {

	// camera
	// ---------------------------------------------------------

	const float eyeHeight = 1.7f;

	cam.cam_position = player.position + glm::vec3(0.0f, eyeHeight, 0.0f);

	glm::vec3 lookDir;

	lookDir.x = cos(cam.alpha) * cos(cam.beta);
	lookDir.y = sin(cam.beta);
	lookDir.z = sin(cam.alpha) * cos(cam.beta);

	lookDir = glm::normalize(lookDir);

	cam.cam_target = cam.cam_position + lookDir;

	cam.cam_up = glm::vec3(0, 1, 0);
}