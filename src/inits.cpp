#include "headers/inits.h"

SDL_Window* window;
SDL_GLContext context;

void init_viewport() {

	glGenFramebuffers(1, &viewportFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

	Create_viewport_tex(viewport_width, viewport_height);

	Create_depth_tex(viewport_width, viewport_height);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer incompleto\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_grid() {
	
	Create_Grid();
}

void init_skybox(const char* skyboxFile) {
	
	GenSkybox();

	Load_Skybox(skyboxFile);

	gen_cubemap();

	ConvertHDRToCubemap();
}

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

			glm::vec3 min(FLT_MAX);
			glm::vec3 max(-FLT_MAX);

			for (const Vertex& v : obj.vertices) {

				min = glm::min(min, v.position);
				max = glm::max(max, v.position);
			}

			obj.min = min;
			obj.max = max;

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


void init_(const char* settings) {

	par.load(settings);

	windowedSize = { par.w, par.h };

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	window = SDL_CreateWindow(par.win_name.c_str(), par.w, par.h, SDL_WINDOW_OPENGL);

	ApplyWindowMode(window);

	context = SDL_GL_CreateContext(window);

	if (!par.VSync)
		SDL_GL_SetSwapInterval(0);
	else
		SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {

		std::cout << glewGetErrorString(err) << '\n';
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGuiStyle& style = ImGui::GetStyle();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init("#version 330");

	create_programs();

	init();
}

void open_init(const char* settings) {

	selectedObject = -1;

	for (auto& o : objects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DeleteObjectGPU(o);
	}

	objects.clear();

	objectNameCounters.clear();

	par.load(settings);

	init();
}