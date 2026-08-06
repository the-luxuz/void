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

void init_skybox(const char* skyboxFile, bool create_vaos) {
	
	if (create_vaos)
		GenSkybox();

	Load_Skybox(skyboxFile);

	if (create_vaos)
		gen_cubemap();

	ConvertHDRToCubemap();
}

void init_camera() {

	cam.fov = st.fov;

	glm::vec3 offset = cam.cam_position - cam.cam_target;

	cam.radius = glm::length(offset);

	cam.alpha = acos(offset.y / cam.radius);
	cam.beta = atan2(offset.z, offset.x);
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

void init_obj(const char* objFile, Light& obj) {

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

bool init_lights(Light& lgh) {

	init_obj("../data/objects/sphere.obj", lgh);

	return true;
}

void init() {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	if(!dont_reload_cam)
		init_camera();

	init_grid();

	init_skybox(st.skybox.c_str(), true);

	init_viewport();

	read_textures_path();

	ReloadFolder();

	objects.clear();

	for (auto& p : par.objects) {

		Object obj;

		init_obj(p.mesh.c_str(), obj);

		Build_Normals(obj);
		Build_TBN(obj);
		CreateNormalBuffers(obj);

		buffer(obj);

		obj.name = p.obj_name;
		obj.mesh = p.mesh;
		obj.tex = p.tex;

		obj.position = p.position;
		obj.rotation = p.rotation;
		obj.scale = p.scale;

		obj.coordType = p.coordType;
		obj.cyaxisType = p.cyaxisType;
		obj.mapping = p.mapping;

		obj.max = p.max;
		obj.min = p.min;

		obj.obj_texture = TextureManager::Get(obj.tex);

		buffer(obj);

		obj.original_vertices = obj.vertices;
		obj.original_indices = obj.indices;

		UpdateMapping(obj);

		LoadNormalMap(obj);
		objects.push_back(std::move(obj));
	}

	for (int i = 0; i < par.lights.size() && i < 8; i++) { // up to 8 lights

		Light light;

		Object obj;

		if (init_lights(light)) {

			light.name = par.lights[i].name;

			light.pos = par.lights[i].pos;
			light.dir = par.lights[i].dir;
			light.col = par.lights[i].col;
			light.att = par.lights[i].att;

			light.ambient = par.lights[i].amb;

			light.bias = par.lights[i].bias;
			light.pcf = par.lights[i].pcf;

			light.inner = par.lights[i].inner;
			light.outer = par.lights[i].outer;
			light.falloff = par.lights[i].falloff;

			light.type = par.lights[i].type;

			if (light.type == "POINT")
				light.typeID = 0;

			else if (light.type == "DIR")
				light.typeID = 1;

			else if (light.type == "SPOT")
				light.typeID = 2;

			obj.position = light.pos;
			obj.rotation = glm::vec3(0);
			obj.scale = glm::vec3(2);

			obj.vertices = light.vertices;
			obj.indices = light.indices;

			buffer(obj);

			lights.push_back(light);

			lightObjects.push_back(obj);
		}
	}

	Light light;

	Object obj;

	if (init_lights(light)) {

		light.name = "Light";

		light.pos = glm::vec3(100.0f);
		light.dir = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
		light.col = glm::vec3(1.0f, 1.0f, 1.0f);
		light.att = glm::vec3(0.f);

		light.att = glm::vec3(1.0f, 0.09f, 0.032f);

		light.ambient = 0.15f;

		light.bias = 0.00001f;
		light.pcf = 3.0f;

		light.inner = 10.0f;
		light.outer = 40.0f;
		light.falloff = 1.0f;

		light.type = "SPOT";

		light.typeID = 0;

		obj.position = light.pos;
		obj.rotation = glm::vec3(0.0f);
		obj.scale = glm::vec3(20.f);

		obj.vertices = light.vertices;
		obj.indices = light.indices;

		buffer(obj);

		lights.push_back(light);
		lightObjects.push_back(obj);
	}
}


void init_(const char* settings) {

	//st.win_name = "void";
	//st.win_mode = window_mode::WINDOWED;
	//st.w = 1280;
	//st.h = 720;
	//st.VSync = 0;
	//st.fov = 90;
	//
	//par.save(settings, st, objects, cam);

	if (par.load(settings, st, cam)) {

		windowedSize = { st.w, st.h };

		SDL_Init(SDL_INIT_VIDEO);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		window = SDL_CreateWindow(st.win_name.c_str(), st.w, st.h, SDL_WINDOW_OPENGL);

		ApplyWindowMode(window);

		context = SDL_GL_CreateContext(window);

		if (!st.VSync)
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
}

void open_init(const char* settings, Settings& st) {

	selectedObject = -1;

	for (auto& o : objects) {

		o.undoStack.clear();
		o.redoStack.clear();

		DeleteObjectGPU(o);
	}

	objects.clear();

	objectNameCounters.clear();

	if(par.load(settings, st, cam))
		init();
}